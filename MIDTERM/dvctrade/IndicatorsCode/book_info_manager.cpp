/**
    \file IndicatorsCode/book_info_manager.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#include <sstream>

#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/book_info_manager.hpp"

namespace HFSAT {

std::map<std::string, BookInfoManager*> BookInfoManager::concise_indicator_description_map_;

BookInfoManager* BookInfoManager::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                    SecurityMarketView& _indep_market_view_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname();

  std::string concise_indicator_description_(t_temp_oss_.str());

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new BookInfoManager(t_dbglogger_, r_watch_, _indep_market_view_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

void BookInfoManager::RemoveUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                           SecurityMarketView& _indep_market_view_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname();

  std::string concise_indicator_description_(t_temp_oss_.str());
  auto iter = concise_indicator_description_map_.find(concise_indicator_description_);
  if (iter != concise_indicator_description_map_.end()) {
    delete iter->second;
    concise_indicator_description_map_.erase(iter);
  }
}

BookInfoManager::BookInfoManager(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                 SecurityMarketView& _indep_market_view_)
    : dbglogger_(t_dbglogger_), watch_(r_watch_), indep_market_view_(_indep_market_view_), max_num_levels_(0) {
  last_decay_factor_update_msecs_ = 0;
  InitializeValues();
  indep_market_view_.subscribe_L2(this);
}

void BookInfoManager::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& cr_market_update_info_) {
  if (cr_market_update_info_.bestbid_int_price_ != indep_market_view_.bid_int_price(0) ||
      cr_market_update_info_.bestask_int_price_ != indep_market_view_.ask_int_price(0)) {
    return;
  }

  if (indep_market_view_.market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_) {
    uint16_t bid_level_change_bitmask_ = 0;
    uint16_t ask_level_change_bitmask_ = 0;

    if (indep_market_view_.spread_increments() != last_spread_increments_) {
      recompute_bid_factors_ = true;
      recompute_ask_factors_ = true;
      last_spread_increments_ = indep_market_view_.spread_increments();
    } else {
      bid_level_change_bitmask_ = indep_market_view_.bid_level_change_bitmask_ % (1 << max_num_levels_);
      ask_level_change_bitmask_ = indep_market_view_.ask_level_change_bitmask_ % (1 << max_num_levels_);

      if (bid_level_change_bitmask_ == modify_all_bits_ ||
          (indep_market_view_.bid_access_bitmask_ & indep_market_view_.bitmask_access_lookup_table_[max_num_levels_]) !=
              indep_market_view_.bitmask_access_lookup_table_[max_num_levels_]) {
        recompute_bid_factors_ = true;
      }

      if (ask_level_change_bitmask_ == modify_all_bits_ ||
          (indep_market_view_.ask_access_bitmask_ & indep_market_view_.bitmask_access_lookup_table_[max_num_levels_]) !=
              indep_market_view_.bitmask_access_lookup_table_[max_num_levels_]) {
        recompute_ask_factors_ = true;
      }
    }

    if (recompute_bid_factors_ || indep_market_view_.market_update_info_.compute_stable_levels_) {
      for (size_t i = 0; i < book_info_struct_.size(); i++) {
        book_info_struct_[i]->ResetBidVariables();
      }

      for (size_t i = 0; i < bid_size_.size(); i++) {
        bid_size_[i] = 0;
        bid_price_[i] = 0;
        bid_ordercount_[i] = 0;
      }

      for (unsigned int current_bid_index_ = indep_market_view_.GetBaseBidMapIndex(), i = 0;
           i < max_num_levels_ && current_bid_index_ > 0; i++) {
        unsigned int bid_int_price_level_ =
            indep_market_view_.spread_increments() + 2 * (indep_market_view_.GetBaseBidMapIndex() - current_bid_index_);
        if (bid_int_price_level_ >= (2 * max_num_levels_)) {
          break;
        }

        const MarketUpdateInfoLevelStruct& this_bid_level_ = indep_market_view_.GetBidLevelAtIndex(current_bid_index_);

        ComputeBidVariables(this_bid_level_, i, bid_int_price_level_);
        if (indep_market_view_.market_update_info_.compute_stable_levels_) {
          int bid_size_ = 0, bid_order_ = 0;
          current_bid_index_ = indep_market_view_.get_next_stable_bid_level_index(
              current_bid_index_, indep_market_view_.level_size_thresh_, bid_size_, bid_order_);
        } else {
          current_bid_index_ = indep_market_view_.GetNextBidMapIndex(current_bid_index_);
        }
      }

      if ((indep_market_view_.bid_access_bitmask_ & indep_market_view_.bitmask_access_lookup_table_[max_num_levels_]) ==
          indep_market_view_.bitmask_access_lookup_table_[max_num_levels_]) {
        recompute_bid_factors_ = false;
      }
    } else {
      while (bid_level_change_bitmask_ > 0) {
        int index_ = ffs(bid_level_change_bitmask_) - 1;
        unsigned int current_bid_index_ =
            indep_market_view_.GetBaseBidMapIndex() -
            index_;  // for stable levels Get the current_bid_index in in which index_ falls
        const MarketUpdateInfoLevelStruct& this_bid_level_ = indep_market_view_.GetBidLevelAtIndex(current_bid_index_);

        unsigned int bid_int_price_level_ = indep_market_view_.spread_increments() + 2 * index_;
        if (bid_int_price_level_ >= (2 * max_num_levels_)) {
          break;
        }

        ComputeBidVariables(this_bid_level_, index_, bid_int_price_level_);

        bid_level_change_bitmask_ ^= (1 << index_);
      }
    }

    if (recompute_ask_factors_ || indep_market_view_.market_update_info_.compute_stable_levels_) {
      for (size_t i = 0; i < book_info_struct_.size(); i++) {
        book_info_struct_[i]->ResetAskVariables();
      }

      for (size_t i = 0; i < bid_size_.size(); i++) {
        ask_size_[i] = 0;
        ask_price_[i] = 0;
        ask_ordercount_[i] = 0;
      }

      for (unsigned int current_ask_index_ = indep_market_view_.GetBaseAskMapIndex(), i = 0;
           i < max_num_levels_ && current_ask_index_ > 0; i++) {
        unsigned int ask_int_price_level_ =
            indep_market_view_.spread_increments() + 2 * (indep_market_view_.GetBaseAskMapIndex() - current_ask_index_);
        if (ask_int_price_level_ >= (2 * max_num_levels_)) {
          break;
        }

        const MarketUpdateInfoLevelStruct& this_ask_level_ = indep_market_view_.GetAskLevelAtIndex(current_ask_index_);

        ComputeAskVariables(this_ask_level_, i, ask_int_price_level_);

        if (indep_market_view_.market_update_info_.compute_stable_levels_) {
          int ask_size_ = 0, ask_order_ = 0;
          current_ask_index_ = indep_market_view_.get_next_stable_ask_level_index(
              current_ask_index_, indep_market_view_.level_size_thresh_, ask_size_, ask_order_);
        } else {
          current_ask_index_ = indep_market_view_.GetNextAskMapIndex(current_ask_index_);
        }
      }

      if ((indep_market_view_.ask_access_bitmask_ & indep_market_view_.bitmask_access_lookup_table_[max_num_levels_]) ==
          indep_market_view_.bitmask_access_lookup_table_[max_num_levels_]) {
        recompute_ask_factors_ = false;
      }
    } else {
      while (ask_level_change_bitmask_ > 0) {
        int index_ = ffs(ask_level_change_bitmask_) - 1;
        unsigned int current_ask_index_ = indep_market_view_.GetBaseAskMapIndex() - index_;
        const MarketUpdateInfoLevelStruct& this_ask_level_ = indep_market_view_.GetAskLevelAtIndex(current_ask_index_);

        unsigned int ask_int_price_level_ = indep_market_view_.spread_increments() + 2 * index_;
        if (ask_int_price_level_ >= (2 * max_num_levels_)) {
          break;
        }

        ComputeAskVariables(this_ask_level_, index_, ask_int_price_level_);

        ask_level_change_bitmask_ ^= (1 << index_);
      }
    }
  } else {
    unsigned int base_level_ = indep_market_view_.spread_increments();

    for (size_t i = 0; i < book_info_struct_.size(); i++) {
      book_info_struct_[i]->ResetBidVariables();
      book_info_struct_[i]->ResetAskVariables();
    }

    for (size_t i = 0; i < bid_size_.size(); i++) {
      bid_size_[i] = 0;
      bid_price_[i] = 0;
      bid_ordercount_[i] = 0;

      ask_size_[i] = 0;
      ask_price_[i] = 0;
      ask_ordercount_[i] = 0;
    }

    for (auto i = 0u; i < max_num_levels_ && i < indep_market_view_.bidlevels_size(); i++) {
      unsigned int this_bid_int_price_level_ = base_level_ + 2 * indep_market_view_.bid_int_price_level(i);
      if (this_bid_int_price_level_ >= (2 * max_num_levels_)) {
        break;
      }

      ComputeBidVariables(indep_market_view_.market_update_info_.bidlevels_[i], i, this_bid_int_price_level_);
    }

    for (auto i = 0u; i < max_num_levels_ && i < indep_market_view_.asklevels_size(); i++) {
      unsigned int this_ask_int_price_level_ = base_level_ + 2 * indep_market_view_.ask_int_price_level(i);
      if (this_ask_int_price_level_ >= (2 * max_num_levels_)) {
        break;
      }

      ComputeAskVariables(indep_market_view_.market_update_info_.asklevels_[i], i, this_ask_int_price_level_);
    }
  }
}

void BookInfoManager::OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& cr_trade_print_info_,
                                   const MarketUpdateInfo& cr_market_update_info_) {}

void BookInfoManager::ComputeSumSize(const unsigned int t_num_levels_, const double t_decay_factor_,
                                     const double stdev_duration) {
  GetBookInfoStruct(t_num_levels_, t_decay_factor_, stdev_duration)->computing_sum_size_ = true;
}

void BookInfoManager::ComputeSumFactorSize(const unsigned int t_num_levels_, const double t_decay_factor_,
                                           const double stdev_duration) {
  GetBookInfoStruct(t_num_levels_, t_decay_factor_, stdev_duration)->computing_sum_factor_size_ = true;
}

void BookInfoManager::ComputeSumPrice(const unsigned int t_num_levels_, const double t_decay_factor_,
                                      const double stdev_duration) {
  GetBookInfoStruct(t_num_levels_, t_decay_factor_, stdev_duration)->computing_sum_price_ = true;
}

void BookInfoManager::ComputeSumFactorPriceSize(const unsigned int t_num_levels_, const double t_decay_factor_,
                                                const double stdev_duration) {
  GetBookInfoStruct(t_num_levels_, t_decay_factor_, stdev_duration)->computing_sum_factor_price_size_ = true;
}

void BookInfoManager::ComputeSumFactorOrder(const unsigned int t_num_levels_, const double t_decay_factor_,
                                            const double stdev_duration) {
  GetBookInfoStruct(t_num_levels_, t_decay_factor_, stdev_duration)->computing_sum_factor_order_ = true;
}

void BookInfoManager::ComputeSumFactorPriceOrder(const unsigned int t_num_levels_, const double t_decay_factor_,
                                                 const double stdev_duration) {
  GetBookInfoStruct(t_num_levels_, t_decay_factor_, stdev_duration)->computing_sum_factor_price_order_ = true;
}

void BookInfoManager::ComputeSumFactor(const unsigned int t_num_levels_, const double t_decay_factor_,
                                       const double stdev_duration) {
  GetBookInfoStruct(t_num_levels_, t_decay_factor_, stdev_duration)->computing_sum_factor_ = true;
}

inline void BookInfoManager::ComputeBidVariables(const MarketUpdateInfoLevelStruct& t_bid_level_,
                                                 const unsigned int t_index_, const unsigned int t_int_price_level_) {
  int this_level_size_ = 0;
  int this_level_order_ = 0;
  if (indep_market_view_.market_update_info_.compute_stable_levels_) {
    this_level_size_ = indep_market_view_.bid_size(t_index_);
    this_level_order_ = indep_market_view_.bid_order(t_index_);
  } else {
    this_level_size_ = t_bid_level_.limit_size_;
    this_level_order_ = t_bid_level_.limit_ordercount_;
  }

  for (size_t i = 0; i < book_info_struct_.size(); i++) {
    if (t_int_price_level_ < 2 * book_info_struct_[i]->num_levels_) {
      if (book_info_struct_[i]->computing_sum_factor_size_) {
        book_info_struct_[i]->sum_bid_factor_size_ +=
            book_info_struct_[i]->decay_vector_[t_int_price_level_] * (this_level_size_ - bid_size_[t_index_]);
      }

      if (book_info_struct_[i]->computing_sum_size_) {
        book_info_struct_[i]->sum_bid_size_ += (this_level_size_ - bid_size_[t_index_]);
      }

      if (book_info_struct_[i]->computing_sum_price_) {
        book_info_struct_[i]->sum_bid_price_ += (t_bid_level_.limit_price_ - bid_price_[t_index_]);
      }

      if (book_info_struct_[i]->computing_sum_factor_price_size_) {
        book_info_struct_[i]->sum_bid_factor_price_size_ += book_info_struct_[i]->decay_vector_[t_int_price_level_] *
                                                            t_bid_level_.limit_price_ *
                                                            (this_level_size_ - bid_size_[t_index_]);
      }

      if (book_info_struct_[i]->computing_sum_factor_order_) {
        book_info_struct_[i]->sum_bid_factor_order_ +=
            book_info_struct_[i]->decay_vector_[t_int_price_level_] * (this_level_order_ - bid_ordercount_[t_index_]);
      }

      if (book_info_struct_[i]->computing_sum_factor_price_order_) {
        book_info_struct_[i]->sum_bid_factor_price_order_ += book_info_struct_[i]->decay_vector_[t_int_price_level_] *
                                                             t_bid_level_.limit_price_ *
                                                             (this_level_order_ - bid_ordercount_[t_index_]);
      }

      if (book_info_struct_[i]->computing_sum_factor_) {
        book_info_struct_[i]->sum_bid_factor_ += book_info_struct_[i]->decay_vector_[t_int_price_level_] *
                                                 ((this_level_size_ > 0 ? 1 : 0) - bid_size_[t_index_] > 0 ? 1 : 0);
      }
    }
  }

  if (indep_market_view_.market_update_info_.compute_stable_levels_) {
    bid_size_[t_index_] = this_level_size_;
    bid_price_[t_index_] = t_bid_level_.limit_price_;
    bid_ordercount_[t_index_] = this_level_order_;
  } else {
    bid_size_[t_index_] = t_bid_level_.limit_size_;
    bid_price_[t_index_] = t_bid_level_.limit_price_;
    bid_ordercount_[t_index_] = t_bid_level_.limit_ordercount_;
  }
}

inline void BookInfoManager::ComputeAskVariables(const MarketUpdateInfoLevelStruct& t_ask_level_,
                                                 const unsigned int t_index_, const unsigned int t_int_price_level_) {
  int this_level_size_ = 0;
  int this_level_ordercount_ = 0;

  if (indep_market_view_.market_update_info_.compute_stable_levels_) {
    this_level_size_ = indep_market_view_.ask_size(t_index_);
    this_level_ordercount_ = indep_market_view_.ask_order(t_index_);
  } else {
    this_level_size_ = t_ask_level_.limit_size_;
    this_level_ordercount_ = t_ask_level_.limit_ordercount_;
  }

  for (size_t i = 0; i < book_info_struct_.size(); i++) {
    if (t_int_price_level_ < 2 * book_info_struct_[i]->num_levels_) {
      if (book_info_struct_[i]->computing_sum_factor_size_) {
        book_info_struct_[i]->sum_ask_factor_size_ +=
            book_info_struct_[i]->decay_vector_[t_int_price_level_] * (this_level_size_ - ask_size_[t_index_]);
      }

      if (book_info_struct_[i]->computing_sum_size_) {
        book_info_struct_[i]->sum_ask_size_ += (this_level_size_ - ask_size_[t_index_]);
      }

      if (book_info_struct_[i]->computing_sum_price_) {
        book_info_struct_[i]->sum_ask_price_ += (t_ask_level_.limit_price_ - ask_price_[t_index_]);
      }

      if (book_info_struct_[i]->computing_sum_factor_price_size_) {
        book_info_struct_[i]->sum_ask_factor_price_size_ += book_info_struct_[i]->decay_vector_[t_int_price_level_] *
                                                            t_ask_level_.limit_price_ *
                                                            (this_level_size_ - ask_size_[t_index_]);
      }

      if (book_info_struct_[i]->computing_sum_factor_order_) {
        book_info_struct_[i]->sum_ask_factor_order_ += book_info_struct_[i]->decay_vector_[t_int_price_level_] *
                                                       (this_level_ordercount_ - ask_ordercount_[t_index_]);
      }

      if (book_info_struct_[i]->computing_sum_factor_price_order_) {
        book_info_struct_[i]->sum_ask_factor_price_order_ += book_info_struct_[i]->decay_vector_[t_int_price_level_] *
                                                             t_ask_level_.limit_price_ *
                                                             (this_level_ordercount_ - ask_ordercount_[t_index_]);
      }

      if (book_info_struct_[i]->computing_sum_factor_) {
        book_info_struct_[i]->sum_ask_factor_ += book_info_struct_[i]->decay_vector_[t_int_price_level_] *
                                                 ((this_level_size_ > 0 ? 1 : 0) - ask_size_[t_index_] > 0 ? 1 : 0);
      }
    }
  }

  if (indep_market_view_.market_update_info_.compute_stable_levels_) {
    ask_size_[t_index_] = this_level_size_;
    ask_price_[t_index_] = t_ask_level_.limit_price_;
    ask_ordercount_[t_index_] = this_level_ordercount_;
  } else {
    ask_size_[t_index_] = t_ask_level_.limit_size_;
    ask_price_[t_index_] = t_ask_level_.limit_price_;
    ask_ordercount_[t_index_] = t_ask_level_.limit_ordercount_;
  }
}

inline void BookInfoManager::ResetLocalConstants(const unsigned int t_num_levels_) {
  if (t_num_levels_ > max_num_levels_) {
    max_num_levels_ = t_num_levels_;

    modify_all_bits_ = 0x0000;
    for (auto i = 0u; i < max_num_levels_; i++) {
      modify_all_bits_ |= (1 << i);
    }

    bid_size_.resize(max_num_levels_, 0);
    bid_price_.resize(max_num_levels_, 0);
    bid_ordercount_.resize(max_num_levels_, 0);

    ask_size_.resize(max_num_levels_, 0);
    ask_price_.resize(max_num_levels_, 0);
    ask_ordercount_.resize(max_num_levels_, 0);
  }
}

BookInfoManager::BookInfoStruct* BookInfoManager::GetBookInfoStruct(const unsigned int t_num_levels_,
                                                                    const double t_decay_factor_,
                                                                    double stdev_duration) {
  for (auto i = 0u; i < book_info_struct_.size(); i++) {
    if (book_info_struct_[i]->num_levels_ == t_num_levels_ &&
        round(book_info_struct_[i]->decay_factor_ * 100) == round(t_decay_factor_ * 100) &&
        (fabs(book_info_struct_[i]->stdev_duration_ - stdev_duration) < 0.000001)) {
      return book_info_struct_[i];
    }
  }

  ResetLocalConstants(t_num_levels_);

  bool scale_level = stdev_duration > 0.000001;
  BookInfoStruct* new_book_info_struct = new BookInfoStruct(t_num_levels_, t_decay_factor_, scale_level);
  if (scale_level) {
    new_book_info_struct->stdev_duration_ = stdev_duration;
    InitializeDynamicValues(new_book_info_struct, stdev_duration);
  }
  book_info_struct_.push_back(new_book_info_struct);

  return new_book_info_struct;
}

/*void BookInfoManager::OnTimePeriodUpdate(const int num_pages_to_add) {
  for (auto& book_info : book_info_struct_) {
    if (book_info->allow_scaling_) {
      // scale the levels only for required book-info
      double current_stdev = book_info->stdev_calculator_->stdev_value();
      // double new_decay_factor = std::max(0.01, std::min(1.00, book_info->hist_stdev_ / current_stdev));

      // capping the StdevRatio value
      double current_dep_stdev_ratio = std::min(STDEV_RATIO_CAP, current_stdev / book_info->hist_stdev_);
      for (auto i = 0u; i < book_info->decay_vector_.size(); i++) {
        int fact = (int)((double)i / current_dep_stdev_ratio);
        book_info->decay_vector_[i] = pow(sqrt(book_info->decay_factor_), (int)fact);
      }

      // we can directly set the num_levels to maximum possible value given input num_levels
      int new_num_levels = current_dep_stdev_ratio * book_info->num_levels_;
      ResetLocalConstants(new_num_levels);
    }
  }
}*/

void BookInfoManager::OnStdevRatioUpdate(const unsigned int index_to_send, const double& r_new_scaled_volume_value) {
  if (index_to_send == 0u) {
    if (watch_.msecs_from_midnight() - last_decay_factor_update_msecs_ >= 600000) {
      for (auto& book_info : book_info_struct_) {
        if (book_info->allow_scaling_) {
          book_info->current_dep_stdev_ratio_ =
              std::max(0.5, std::min(STDEV_RATIO_CAP, book_info->stdev_ratio_calculator_->stdev_ratio()));

          for (auto i = 0u; i < book_info->decay_vector_.size(); i++) {
            int fact = (int)((double)i / book_info->current_dep_stdev_ratio_);
            book_info->decay_vector_[i] = pow(sqrt(book_info->decay_factor_), (int)fact);
          }

          int new_num_levels = book_info->current_dep_stdev_ratio_ * book_info->num_levels_;
          ResetLocalConstants(new_num_levels);
        }
      }
      last_decay_factor_update_msecs_ = watch_.msecs_from_midnight();
    }
  }
}
void BookInfoManager::InitializeValues() {}

void BookInfoManager::InitializeDynamicValues(BookInfoStruct* book_info, double stdev_duration) {
  // Using SlowStdevCalculator instead of StdevRatioCalculator as StdevRatioCalculator uses sample data to normalize the
  // stdev for time-period,
  // For changing number of levels based on stdev, stdev should be normalized with some fixed value rather than moving

  book_info->stdev_ratio_calculator_ =
      StdevRatioCalculator::GetUniqueInstance(dbglogger_, watch_, indep_market_view_, stdev_duration, false);

  // 300 moving hist_stdev value
  // book_info->hist_stdev_ = PcaWeightsManager::GetUniqueInstance().GetShortcodeStdevs(indep_market_view_.shortcode());
  // book_info->hist_stdev_ *= sqrt((stdev_duration / 300.0));

  book_info->stdev_ratio_calculator_->AddStdevRatioListener(0u, this);
  // make sure we receive time-period update to update num_levels
  // watch_.subscribe_FifteenMinutesPeriod(this);
}
}
