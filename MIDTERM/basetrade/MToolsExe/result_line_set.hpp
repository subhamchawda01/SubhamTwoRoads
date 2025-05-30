/**
   \file MToolsExe/result_line_set.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */

#pragma once

#include <map>
#include <algorithm>
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CommonDataStructures/vector_utils_weighted.hpp"

namespace HFSAT {

inline double sqrtSign(double value_) { return ((value_ < 0) ? -1 : 1) * sqrt(fabs(value_)); }

struct ResultLine {
  int yyyymmdd_;
  int pnl_;
  int volume_;
  int supporting_order_filled_percent_;
  int best_level_order_filled_percent_;
  int aggressive_order_filled_percent_;
  int improve_order_filled_percent_;
  // extra stats
  double average_abs_position_;
  int median_time_to_close_trades_;
  int average_time_to_close_trades_;
  int max_time_to_close_trades_;
  int median_closed_trade_pnls_;
  int average_closed_trade_pnls_;
  double stdev_closed_trade_pnls_;
  double sharpe_closed_trade_pnls_;
  double fracpos_closed_trade_pnls_;

  int min_pnl_;
  int max_pnl_;
  int min_adjusted_pnl_;
  int max_drawdown_;

  double pnl_zscore_;

  int msg_count_;
  int volume_normalized_average_time_to_close_trades_;
  int num_opentrade_hits_;
  double abs_closing_position_;
  int unit_trade_size_;
  int ptrades_;
  int ttrades_;
  int total_trades;

  ResultLine()
      : yyyymmdd_(0),
        pnl_(-100),
        volume_(0),
        supporting_order_filled_percent_(0),
        best_level_order_filled_percent_(100),
        aggressive_order_filled_percent_(0),
        improve_order_filled_percent_(0),
        average_abs_position_(0.0),
        median_time_to_close_trades_(0),
        average_time_to_close_trades_(0),
        max_time_to_close_trades_(0),
        median_closed_trade_pnls_(-100),
        average_closed_trade_pnls_(-100),
        stdev_closed_trade_pnls_(0.00),
        sharpe_closed_trade_pnls_(-100),
        fracpos_closed_trade_pnls_(0),
        min_pnl_(0),
        max_pnl_(0),
        min_adjusted_pnl_(0),
        max_drawdown_(0),
        pnl_zscore_(0),
        msg_count_(0),
        volume_normalized_average_time_to_close_trades_(1000),
        num_opentrade_hits_(0),
        abs_closing_position_(0.0),
        unit_trade_size_(0),
        ptrades_(0),
        ttrades_(0),
        total_trades(0)	{}

  ResultLine(int _date_, const std::vector<double>& tokens_)
      : yyyymmdd_(0),
        pnl_(-100),
        volume_(0),
        supporting_order_filled_percent_(0),
        best_level_order_filled_percent_(100),
        aggressive_order_filled_percent_(0),
        improve_order_filled_percent_(0),
        average_abs_position_(0.0),
        median_time_to_close_trades_(0),
        average_time_to_close_trades_(0),
        max_time_to_close_trades_(0),
        median_closed_trade_pnls_(-100),
        average_closed_trade_pnls_(-100),
        stdev_closed_trade_pnls_(0.00),
        sharpe_closed_trade_pnls_(-100),
        fracpos_closed_trade_pnls_(0),
        min_pnl_(0),
        max_pnl_(0),
        min_adjusted_pnl_(0),
        max_drawdown_(0),
        pnl_zscore_(0),
        msg_count_(0),
        volume_normalized_average_time_to_close_trades_(1000),
        num_opentrade_hits_(0),
        abs_closing_position_(0.0),
        unit_trade_size_(0),
        ptrades_(0),
        ttrades_(0),
        total_trades(0)	{
    yyyymmdd_ = _date_;
    unsigned int simout_start_idx_ = 0u;
    if (tokens_.size() > simout_start_idx_ + 1u) {
      pnl_ = tokens_[simout_start_idx_ + 0u];
      volume_ = tokens_[simout_start_idx_ + 1u];
    }
    if (tokens_.size() > simout_start_idx_ + 5u) {
      supporting_order_filled_percent_ = tokens_[simout_start_idx_ + 2u];
      best_level_order_filled_percent_ = tokens_[simout_start_idx_ + 3u];
      aggressive_order_filled_percent_ = tokens_[simout_start_idx_ + 4u];
      improve_order_filled_percent_ = tokens_[simout_start_idx_ + 5u];
    }

    unsigned int pnl_stats_start_idx_ = simout_start_idx_ + 6u;  // assuming pnl, volume, sup%, bestlevel%, agg%, imp%
    // other statistics
    if (tokens_.size() > pnl_stats_start_idx_ + 7u) {
      average_abs_position_ = tokens_[pnl_stats_start_idx_ + 0u];
      median_time_to_close_trades_ = tokens_[pnl_stats_start_idx_ + 1u];
      average_time_to_close_trades_ = tokens_[pnl_stats_start_idx_ + 2u];
      median_closed_trade_pnls_ = tokens_[pnl_stats_start_idx_ + 3u];
      average_closed_trade_pnls_ = tokens_[pnl_stats_start_idx_ + 4u];
      stdev_closed_trade_pnls_ = tokens_[pnl_stats_start_idx_ + 5u];
      sharpe_closed_trade_pnls_ = tokens_[pnl_stats_start_idx_ + 6u];
      fracpos_closed_trade_pnls_ = tokens_[pnl_stats_start_idx_ + 7u];
    }
    if (tokens_.size() > pnl_stats_start_idx_ + 9u) {
      min_pnl_ = tokens_[pnl_stats_start_idx_ + 8u];
      max_pnl_ = tokens_[pnl_stats_start_idx_ + 9u];
      min_adjusted_pnl_ = (min_pnl_ + max_pnl_) / 2.0;
    }
    if (tokens_.size() > pnl_stats_start_idx_ + 10u) {
      max_drawdown_ = tokens_[pnl_stats_start_idx_ + 10u];
    }
    if (tokens_.size() > pnl_stats_start_idx_ + 11u) {
      max_time_to_close_trades_ = tokens_[pnl_stats_start_idx_ + 11u];
    }
    if (tokens_.size() > pnl_stats_start_idx_ + 12u) {
      msg_count_ = tokens_[pnl_stats_start_idx_ + 12u];
    }
    volume_normalized_average_time_to_close_trades_ = average_time_to_close_trades_;
    if (tokens_.size() > pnl_stats_start_idx_ + 13u) {
      volume_normalized_average_time_to_close_trades_ = tokens_[pnl_stats_start_idx_ + 13u];
    }
    if (tokens_.size() > pnl_stats_start_idx_ + 14u) {
      num_opentrade_hits_ = tokens_[pnl_stats_start_idx_ + 14u];
    }
    if (tokens_.size() > pnl_stats_start_idx_ + 15u) {
      abs_closing_position_ = tokens_[pnl_stats_start_idx_ + 15u];
    }
    if (tokens_.size() > pnl_stats_start_idx_ + 16u) {
      unit_trade_size_ = tokens_[pnl_stats_start_idx_ + 16u];
    }

    if (tokens_.size() > pnl_stats_start_idx_ + 17u) {
      ptrades_ = tokens_[pnl_stats_start_idx_ + 17u];
    }
    if (tokens_.size() > pnl_stats_start_idx_ + 18u) {
      ttrades_ = tokens_[pnl_stats_start_idx_ + 18u];
    }
    if (tokens_.size() > pnl_stats_start_idx_ + 19u) {
       total_trades = tokens_[pnl_stats_start_idx_ + 19u];
    }
    if (stdev_closed_trade_pnls_ >= 1) {
      pnl_zscore_ = pnl_ / stdev_closed_trade_pnls_;
    } else {
      pnl_zscore_ = 0.001;
    }
  }

  ResultLine(const std::vector<const char*>& tokens_)
      : yyyymmdd_(0),
        pnl_(-100),
        volume_(0),
        supporting_order_filled_percent_(0),
        best_level_order_filled_percent_(100),
        aggressive_order_filled_percent_(0),
        improve_order_filled_percent_(0),
        average_abs_position_(0.0),
        median_time_to_close_trades_(0),
        average_time_to_close_trades_(0),
        max_time_to_close_trades_(0),
        median_closed_trade_pnls_(-100),
        average_closed_trade_pnls_(-100),
        stdev_closed_trade_pnls_(0.00),
        sharpe_closed_trade_pnls_(-100),
        fracpos_closed_trade_pnls_(0),
        min_pnl_(0),
        max_pnl_(0),
        min_adjusted_pnl_(0),
        max_drawdown_(0),
        pnl_zscore_(0),
        msg_count_(0),
        volume_normalized_average_time_to_close_trades_(1000),
        num_opentrade_hits_(0),
        abs_closing_position_(0),
        unit_trade_size_(0),
        ptrades_(0),
        ttrades_(0),
        total_trades(0)	{
    yyyymmdd_ = atoi(tokens_[1]);
    unsigned int simout_start_idx_ = 2u;
    if (tokens_.size() > simout_start_idx_ + 1u) {
      pnl_ = atoi(tokens_[simout_start_idx_ + 0u]);
      volume_ = atoi(tokens_[simout_start_idx_ + 1u]);
    }
    if (tokens_.size() > simout_start_idx_ + 5u) {
      supporting_order_filled_percent_ = atoi(tokens_[simout_start_idx_ + 2u]);
      best_level_order_filled_percent_ = atoi(tokens_[simout_start_idx_ + 3u]);
      aggressive_order_filled_percent_ = atoi(tokens_[simout_start_idx_ + 4u]);
      improve_order_filled_percent_ = atoi(tokens_[simout_start_idx_ + 5u]);
    }

    unsigned int pnl_stats_start_idx_ = simout_start_idx_ + 6u;  // assuming pnl, volume, sup%, bestlevel%, agg%, imp%
    // other statistics
    if (tokens_.size() > pnl_stats_start_idx_ + 7u) {
      average_abs_position_ = atof(tokens_[pnl_stats_start_idx_ + 0u]);
      median_time_to_close_trades_ = atoi(tokens_[pnl_stats_start_idx_ + 1u]);
      average_time_to_close_trades_ = atoi(tokens_[pnl_stats_start_idx_ + 2u]);
      median_closed_trade_pnls_ = atoi(tokens_[pnl_stats_start_idx_ + 3u]);
      average_closed_trade_pnls_ = atoi(tokens_[pnl_stats_start_idx_ + 4u]);
      stdev_closed_trade_pnls_ = atoi(tokens_[pnl_stats_start_idx_ + 5u]);
      sharpe_closed_trade_pnls_ = atof(tokens_[pnl_stats_start_idx_ + 6u]);
      fracpos_closed_trade_pnls_ = atof(tokens_[pnl_stats_start_idx_ + 7u]);
    }
    if (tokens_.size() > pnl_stats_start_idx_ + 9u) {
      min_pnl_ = atoi(tokens_[pnl_stats_start_idx_ + 8u]);
      max_pnl_ = atoi(tokens_[pnl_stats_start_idx_ + 9u]);
      min_adjusted_pnl_ = (min_pnl_ + max_pnl_) / 2.0;
    }
    if (tokens_.size() > pnl_stats_start_idx_ + 10u) {
      max_drawdown_ = atoi(tokens_[pnl_stats_start_idx_ + 10u]);
    }
    if (tokens_.size() > pnl_stats_start_idx_ + 11u) {
      max_time_to_close_trades_ = atoi(tokens_[pnl_stats_start_idx_ + 11u]);
    }
    if (tokens_.size() > pnl_stats_start_idx_ + 12u) {
      msg_count_ = atoi(tokens_[pnl_stats_start_idx_ + 12u]);
    }
    volume_normalized_average_time_to_close_trades_ = average_time_to_close_trades_;
    if (tokens_.size() > pnl_stats_start_idx_ + 13u) {
      volume_normalized_average_time_to_close_trades_ = atoi(tokens_[pnl_stats_start_idx_ + 13u]);
    }
    if (tokens_.size() > pnl_stats_start_idx_ + 14u) {
      num_opentrade_hits_ = atoi(tokens_[pnl_stats_start_idx_ + 14u]);
    }
    if (tokens_.size() > pnl_stats_start_idx_ + 15u) {
      abs_closing_position_ = atof(tokens_[pnl_stats_start_idx_ + 15u]);
    }
    if (tokens_.size() > pnl_stats_start_idx_ + 16u) {
      unit_trade_size_ = atoi(tokens_[pnl_stats_start_idx_ + 16u]);
    }

    if (tokens_.size() > pnl_stats_start_idx_ + 17u) {
      ptrades_ = atoi(tokens_[pnl_stats_start_idx_ + 17u]);
    }
    if (tokens_.size() > pnl_stats_start_idx_ + 18u) {
      ttrades_ = atoi(tokens_[pnl_stats_start_idx_ + 18u]);
    }
    if (tokens_.size() > pnl_stats_start_idx_ + 19u) {
      total_trades = atoi(tokens_[pnl_stats_start_idx_ + 19u]);
    }
    if (stdev_closed_trade_pnls_ >= 1) {
      pnl_zscore_ = pnl_ / stdev_closed_trade_pnls_;
    } else {
      pnl_zscore_ = 0.001;
    }
  }

  void AddResultLine(const ResultLine& src_result_line_) {
    pnl_ += src_result_line_.pnl_;

    if (volume_ + src_result_line_.volume_ > 0) {
      // changing these before we change volume
      supporting_order_filled_percent_ =
          ((supporting_order_filled_percent_ * volume_ +
            src_result_line_.supporting_order_filled_percent_ * src_result_line_.volume_) /
           (volume_ + src_result_line_.volume_));
      best_level_order_filled_percent_ =
          ((best_level_order_filled_percent_ * volume_ +
            src_result_line_.best_level_order_filled_percent_ * src_result_line_.volume_) /
           (volume_ + src_result_line_.volume_));
      aggressive_order_filled_percent_ =
          ((aggressive_order_filled_percent_ * volume_ +
            src_result_line_.aggressive_order_filled_percent_ * src_result_line_.volume_) /
           (volume_ + src_result_line_.volume_));
      improve_order_filled_percent_ = ((improve_order_filled_percent_ * volume_ +
                                        src_result_line_.improve_order_filled_percent_ * src_result_line_.volume_) /
                                       (volume_ + src_result_line_.volume_));

      average_abs_position_ = (average_abs_position_ + src_result_line_.average_abs_position_) / 2.0;
      median_time_to_close_trades_ =
          (median_time_to_close_trades_ + src_result_line_.median_time_to_close_trades_) / 2.0;
      average_time_to_close_trades_ =
          (average_time_to_close_trades_ + src_result_line_.average_time_to_close_trades_) / 2.0;
      median_closed_trade_pnls_ = (median_closed_trade_pnls_ * volume_ +
                                   src_result_line_.median_closed_trade_pnls_ * src_result_line_.volume_) /
                                  (volume_ + src_result_line_.volume_);
      average_closed_trade_pnls_ = (average_closed_trade_pnls_ * volume_ +
                                    src_result_line_.average_closed_trade_pnls_ * src_result_line_.volume_) /
                                   (volume_ + src_result_line_.volume_);
      stdev_closed_trade_pnls_ =
          (stdev_closed_trade_pnls_ * volume_ + src_result_line_.stdev_closed_trade_pnls_ * src_result_line_.volume_) /
          (volume_ + src_result_line_.volume_);
      sharpe_closed_trade_pnls_ = (sharpe_closed_trade_pnls_ * volume_ +
                                   src_result_line_.sharpe_closed_trade_pnls_ * src_result_line_.volume_) /
                                  (volume_ + src_result_line_.volume_);
      fracpos_closed_trade_pnls_ = (fracpos_closed_trade_pnls_ * volume_ +
                                    src_result_line_.fracpos_closed_trade_pnls_ * src_result_line_.volume_) /
                                   (volume_ + src_result_line_.volume_);

      // no wa to guess min and max ... so just approximating by dividing by sqrt(2)
      min_pnl_ = std::min((double)pnl_, (min_pnl_ + src_result_line_.min_pnl_) / 1.414);
      max_pnl_ = std::max((double)pnl_, (max_pnl_ + src_result_line_.max_pnl_) / 1.414);
      min_adjusted_pnl_ = (min_pnl_ + max_pnl_) / 2.0;

      max_drawdown_ = (max_drawdown_ + src_result_line_.max_drawdown_) / 1.414;
      max_time_to_close_trades_ = std::max(max_time_to_close_trades_, src_result_line_.max_time_to_close_trades_);
      msg_count_ += src_result_line_.msg_count_;
      volume_normalized_average_time_to_close_trades_ =
          (volume_normalized_average_time_to_close_trades_ * volume_ +
           src_result_line_.volume_normalized_average_time_to_close_trades_ * src_result_line_.volume_) /
          (volume_ + src_result_line_.volume_);
      num_opentrade_hits_ += src_result_line_.num_opentrade_hits_;
      abs_closing_position_ += src_result_line_.abs_closing_position_;
      unit_trade_size_ += src_result_line_.unit_trade_size_;
      // ptrades_ = ( ( ptrades_ * ttrades_ ) + ( src_result_line_.ptrades_ * src_result_line_.ttrades_ ) ) / ( ttrades_
      // + src_result_line_.ttrades_ ) ;
      ptrades_ += src_result_line_.ptrades_;
      ttrades_ += src_result_line_.ttrades_;
      if (stdev_closed_trade_pnls_ >= 1) {
        pnl_zscore_ = pnl_ / stdev_closed_trade_pnls_;
      } else {
        pnl_zscore_ = 0.001;
      }

      // volume added up at the end since it is used in normalization
      volume_ += src_result_line_.volume_;
    }
  }
};

inline bool SortResultLineDate(const ResultLine& d1, const ResultLine& d2) { return d1.yyyymmdd_ < d2.yyyymmdd_; }
inline bool SortResultLinePNL(const ResultLine& d1, const ResultLine& d2) { return d1.pnl_ < d2.pnl_; }

typedef std::vector<ResultLine> ResultLineVec;

struct ResultLineSet {
  ResultLineSet()
      : strategy_filename_base_(""),
        result_line_vec_(),
        weights_vec_(),
        pnl_average_(-100),
        pnl_stdev_(1),
        pnl_skewness_(0),
        volume_average_(0),
        pnl_sharpe_(-100),
        pnl_conservative_average_(-100),
        pnl_median_average_(-100),
        pnl_per_contract_(0),
        supporting_order_filled_percent_(0),
        best_level_order_filled_percent_(100),
        aggressive_order_filled_percent_(0),
        improve_order_filled_percent_(0),
        median_average_time_to_close_trades_(1000u),
        average_max_time_to_close_trades_(1000u),
        average_pnl_zscore_(0),
        gain_to_pain_ratio_(0),
        ninety_five_percentile_(0),
        pnl_by_maxloss_(0),
        average_min_adjusted_pnl_(0),
        average_max_drawdown_(0),
        average_abs_position_(0.0),
        dd_adj_pnl_average_(0.00),
        sqrt_dd_adj_pnl_average_(0.00),
        sqrt_dd_ttc_adj_pnl_average_(0.00),
        sqrt_dd_sqrt_ttc_adj_pnl_average_(0.00),
        pnl_sign_bias_(0.0),
        average_msg_count_(0),
        median_volume_normalized_average_time_to_close_trades_(1000),
        average_num_opentrade_hits_(0),
        average_abs_closing_position_(0.0),
        average_unit_trade_size_(0.0),
        percent_positive_trades_(0.0),
        max_cumulative_dd_(0),
        pnl_by_max_cumulative_dd_(0.0),
        longest_negative_stretch_(0),
        cumulative_pnl_(0),
        cumulative_pnl_by_max_dd_(0.0) {}

  std::string strategy_filename_base_;  ///< basename of the strategyfile
  ResultLineVec result_line_vec_;       ///< data of pnls etc
  std::vector<double> weights_vec_;

  // post data stats used to choose the best files
  double pnl_average_;               ///< simple average of pnls
  double pnl_stdev_;                 ///< simple stdev of pnls
  double pnl_skewness_;              ///< sample skewness of PNL series
  double volume_average_;            ///< simple average of volume values
  double pnl_sharpe_;                ///< average / stdev of pnls
  double pnl_conservative_average_;  ///< removing the 25% max pnl days and counting bottom 25% pnl days twice
  double pnl_median_average_;        ///< just the average of the middle 50%
  double pnl_per_contract_;          ///< per unit volume pnl

  int supporting_order_filled_percent_;
  int best_level_order_filled_percent_;
  int aggressive_order_filled_percent_;
  int improve_order_filled_percent_;

  unsigned int median_average_time_to_close_trades_;  ///< median_average of daily average_time_to_close_trades_ values
  unsigned int average_max_time_to_close_trades_;     // Averge of daily max_time_to_close_trades_ values
  double average_pnl_zscore_;                         ///< average of dail pnl_zscore_ values
  double gain_to_pain_ratio_;                         ///< Sum of pnl / - Sum of losses
  double ninety_five_percentile_;                     ///< 95% worst day
  double pnl_by_maxloss_;                             ///< Sum of pnl by ninety_five_percentile_

  double average_min_adjusted_pnl_;          ///< average of min_adjusted_pnl_ values
  double average_max_drawdown_;              ///< average of max_drawdown values
  double average_abs_position_;              ///< average of average_abs_position_ values
  double dd_adj_pnl_average_;                ///< average pnl / average max drawdown
  double sqrt_dd_adj_pnl_average_;           ///< average pnl / sqrt ( average max drawdown )
  double sqrt_dd_ttc_adj_pnl_average_;       // average pnl / ( sqrt ( average max drawdown ) * ttc )
  double sqrt_dd_sqrt_ttc_adj_pnl_average_;  // average pnl / ( sqrt ( average max drawdown ) * sqrt ( ttc ) )
  double pnl_sign_bias_;                     // like sharpe, but here i am fine with +ve values far from mean

  int average_msg_count_;
  unsigned int median_volume_normalized_average_time_to_close_trades_;
  double average_num_opentrade_hits_;
  double average_abs_closing_position_;
  double average_unit_trade_size_;
  double percent_positive_trades_;
  int max_cumulative_dd_;
  double pnl_by_max_cumulative_dd_;
  int longest_negative_stretch_;
  int cumulative_pnl_;
  double cumulative_pnl_by_max_dd_;

  void AddResultSet(const ResultLineSet& src_result_line_set_) {
    if (result_line_vec_.empty()) {
      result_line_vec_ = src_result_line_set_.result_line_vec_;
      weights_vec_ = src_result_line_set_.weights_vec_;
    } else {
      // add the result_line_vec_ to local
      for (unsigned int src_index_ = 0, self_index_ = 0;
           (src_index_ < src_result_line_set_.result_line_vec_.size()) && (self_index_ < result_line_vec_.size());) {
        if (src_result_line_set_.result_line_vec_[src_index_].yyyymmdd_ < result_line_vec_[self_index_].yyyymmdd_) {
          src_index_++;
          result_line_vec_[self_index_] = src_result_line_set_.result_line_vec_[src_index_];
          weights_vec_[self_index_] = src_result_line_set_.weights_vec_[src_index_];
        }
        if (result_line_vec_[self_index_].yyyymmdd_ < src_result_line_set_.result_line_vec_[src_index_].yyyymmdd_) {
          self_index_++;
          // result_line_vec_ [ self_index_ ].pnl_ stays the same
        }
        if (src_result_line_set_.result_line_vec_[src_index_].yyyymmdd_ == result_line_vec_[self_index_].yyyymmdd_) {
          result_line_vec_[self_index_].AddResultLine(src_result_line_set_.result_line_vec_[src_index_]);
          self_index_++;
          src_index_++;
        }
      }
    }
    ComputeStatistics();
  }

  void GetPNLVec(std::vector<double>& pnl_vec_) const {
    pnl_vec_.clear();
    for (auto i = 0u; i < result_line_vec_.size(); i++) {
      pnl_vec_.push_back(result_line_vec_[i].pnl_);
    }
  }

  void GetPNLDateMap(std::map<int, double>& pnl_map_) const {
    pnl_map_.clear();
    for (auto i = 0u; i < result_line_vec_.size(); i++) {
      pnl_map_[result_line_vec_[i].yyyymmdd_] = result_line_vec_[i].pnl_;
    }
  }

  void GetMinPNLVec(std::vector<double>& min_pnl_vec_) const {
    min_pnl_vec_.clear();
    for (auto i = 0u; i < result_line_vec_.size(); i++) {
      min_pnl_vec_.push_back(result_line_vec_[i].min_pnl_);
    }
  }

  void GetMinPNLDateMap(std::map<int, double>& min_pnl_map_) const {
    min_pnl_map_.clear();
    for (auto i = 0u; i < result_line_vec_.size(); i++) {
      min_pnl_map_[result_line_vec_[i].yyyymmdd_] = result_line_vec_[i].min_pnl_;
    }
  }

  void GetVolumeVec(std::vector<double>& volume_vec_) const {
    volume_vec_.clear();
    for (auto i = 0u; i < result_line_vec_.size(); i++) {
      volume_vec_.push_back(result_line_vec_[i].volume_);
    }
  }

  double GetAverageVolume() const {
    std::vector<double> volume_vec_;
    GetVolumeVec(volume_vec_);
    return HFSAT::VectorUtils::GetWeightedMean(volume_vec_, weights_vec_);
  }

  unsigned int GetMedianTimeToCloseTrades() {
    std::vector<unsigned int> mtct_vec_;
    for (auto i = 0u; i < result_line_vec_.size(); i++) {
      mtct_vec_.push_back(std::max(0, result_line_vec_[i].average_time_to_close_trades_));
    }
    sort(mtct_vec_.begin(), mtct_vec_.end());
    return std::max(0u, HFSAT::VectorUtils::GetWeightedMedianAverage(mtct_vec_, true, weights_vec_));
  }

  unsigned int GetAverageMaxTimeToCloseTrades() {
    std::vector<unsigned int> mtct_vec_;
    for (auto i = 0u; i < result_line_vec_.size(); i++) {
      mtct_vec_.push_back(std::max(0, result_line_vec_[i].max_time_to_close_trades_));
    }
    sort(mtct_vec_.begin(), mtct_vec_.end());
    return std::max(0u, HFSAT::VectorUtils::GetWeightedMedianAverage(mtct_vec_, true, weights_vec_));
  }

  unsigned int GetMedianNormalizedTTC() {
    std::vector<unsigned int> normalized_ttc_vec_;
    for (auto i = 0u; i < result_line_vec_.size(); i++) {
      normalized_ttc_vec_.push_back(std::max(0, result_line_vec_[i].volume_normalized_average_time_to_close_trades_));
    }
    return std::max(0u, HFSAT::VectorUtils::GetWeightedMedian(normalized_ttc_vec_, weights_vec_));
  }

  double GetAveragePNLZscore() const {
    std::vector<double> pnlzs_vec_;
    for (auto i = 0u; i < result_line_vec_.size(); i++) {
      pnlzs_vec_.push_back(result_line_vec_[i].pnl_zscore_);
    }
    sort(pnlzs_vec_.begin(), pnlzs_vec_.end());
    return HFSAT::VectorUtils::GetWeightedMedianAverage(pnlzs_vec_, true, weights_vec_);
  }

  double GetAverageMinAdjustedPNL() const {
    std::vector<double> min_adjusted_pnl_vec_;
    for (auto i = 0u; i < result_line_vec_.size(); i++) {
      min_adjusted_pnl_vec_.push_back(result_line_vec_[i].min_adjusted_pnl_);
    }
    return HFSAT::VectorUtils::GetWeightedMean(min_adjusted_pnl_vec_, weights_vec_);
  }

  double GetAverageMaxDrawdown() const {
    std::vector<double> max_drawdown_vec_;
    for (auto i = 0u; i < result_line_vec_.size(); i++) {
      max_drawdown_vec_.push_back(result_line_vec_[i].max_drawdown_);
    }
    return std::max(1.00, HFSAT::VectorUtils::GetWeightedMeanHighestQuartile(max_drawdown_vec_, weights_vec_));
  }

  double GetAverageAbsPosition() const {
    std::vector<double> average_abs_position_vec_;
    for (auto i = 0u; i < result_line_vec_.size(); ++i) {
      average_abs_position_vec_.push_back(result_line_vec_[i].average_abs_position_);
    }
    return HFSAT::VectorUtils::GetWeightedMean(average_abs_position_vec_, weights_vec_);
  }

  int GetAverageMsgCount() const {
    std::vector<int> average_msg_count_vec_;
    for (auto i = 0u; i < result_line_vec_.size(); ++i) {
      average_msg_count_vec_.push_back(result_line_vec_[i].msg_count_);
    }
    return (int)(HFSAT::VectorUtils::GetWeightedMean(average_msg_count_vec_, weights_vec_));
  }

  double GetAverageOpenTradeHits() const {
    std::vector<double> average_num_opentrade_hits_vec_;
    for (auto i = 0u; i < result_line_vec_.size(); ++i) {
      average_num_opentrade_hits_vec_.push_back(result_line_vec_[i].num_opentrade_hits_);
    }
    return (HFSAT::VectorUtils::GetWeightedMean(average_num_opentrade_hits_vec_, weights_vec_));
  }

  double GetAverageAbsClosingPosition() const {
    std::vector<double> average_abs_closing_position_;
    for (auto i = 0u; i < result_line_vec_.size(); ++i) {
      average_abs_closing_position_.push_back(result_line_vec_[i].abs_closing_position_);
    }
    return (HFSAT::VectorUtils::GetWeightedMean(average_abs_closing_position_, weights_vec_));
  }

  double GetAverageUnitTradeSize() const {
    std::vector<double> average_unit_trade_size_vec_;
    for (auto i = 0u; i < result_line_vec_.size(); ++i) {
      average_unit_trade_size_vec_.push_back(result_line_vec_[i].unit_trade_size_);
    }
    return (HFSAT::VectorUtils::GetWeightedMean(average_unit_trade_size_vec_, weights_vec_));
  }

  double GetPnlSignBias() const {
    int npvals_ = 0, nnvals_ = 0;
    for (auto i = 0u; i < result_line_vec_.size(); ++i) {
      if (result_line_vec_[i].pnl_ > 0) {
        npvals_++;
      } else {
        nnvals_++;
      }
    }
    return (std::max(npvals_, nnvals_) / (double)std::max(1, (npvals_ + nnvals_)));
  }

  int GetMaxCumulativeDrawdown() const {
    int max_cumulative_dd_ = 0.0;
    int max_pnl_so_far_ = 0.0;
    int cumulative_pnl_so_far_ = 0;
    for (auto i = 0u; i < result_line_vec_.size(); i++) {
      cumulative_pnl_so_far_ += result_line_vec_[i].pnl_;
      max_pnl_so_far_ = std::max(max_pnl_so_far_, cumulative_pnl_so_far_);
      max_cumulative_dd_ = std::max(max_cumulative_dd_, max_pnl_so_far_ - cumulative_pnl_so_far_);
    }
    return max_cumulative_dd_;
  }

  int GetCumulativePnl() const {
    int cumulative_pnl_so_far_ = 0;
    for (auto i = 0u; i < result_line_vec_.size(); i++) {
      cumulative_pnl_so_far_ += result_line_vec_[i].pnl_;
    }
    return cumulative_pnl_so_far_;
  }

  int GetLongestStretch() {
    int longest_negative_stretch_ = 0;
    int current_stretch_ = 0;
    int current_cumulative_pnl_ = 0;
    int max_pnl_so_far_ = 0;
    for (auto i = 0u; i < result_line_vec_.size(); i++) {
      current_cumulative_pnl_ += result_line_vec_[i].pnl_;
      max_pnl_so_far_ = std::max(max_pnl_so_far_, current_cumulative_pnl_);
      if (current_cumulative_pnl_ < max_pnl_so_far_) {
        current_stretch_++;
        longest_negative_stretch_ = std::max(current_stretch_, longest_negative_stretch_);

      } else {
        current_stretch_ = 0;
      }
    }
    return longest_negative_stretch_;
  }

  double GetPercentPositiveTrades() const {
    int aptrades_ = 0;
    int attrades_ = 0;
    for (auto i = 0u; i < result_line_vec_.size(); ++i) {
      aptrades_ += result_line_vec_[i].ptrades_;
      attrades_ += result_line_vec_[i].ttrades_;
    }
    return (attrades_ > 0 ? (double)aptrades_ / attrades_ : 0);
  }

  void GetFillStats() {
    std::vector<int> supporting_order_filled_percent_vec_;
    std::vector<int> best_level_order_filled_percent_vec_;
    std::vector<int> aggressive_order_filled_percent_vec_;
    std::vector<int> improve_order_filled_percent_vec_;

    for (auto i = 0u; i < result_line_vec_.size(); i++) {
      supporting_order_filled_percent_vec_.push_back(result_line_vec_[i].supporting_order_filled_percent_);
      best_level_order_filled_percent_vec_.push_back(result_line_vec_[i].best_level_order_filled_percent_);
      aggressive_order_filled_percent_vec_.push_back(result_line_vec_[i].aggressive_order_filled_percent_);
      improve_order_filled_percent_vec_.push_back(result_line_vec_[i].improve_order_filled_percent_);
    }

    sort(supporting_order_filled_percent_vec_.begin(), supporting_order_filled_percent_vec_.end());
    sort(best_level_order_filled_percent_vec_.begin(), best_level_order_filled_percent_vec_.end());
    sort(aggressive_order_filled_percent_vec_.begin(), aggressive_order_filled_percent_vec_.end());
    sort(improve_order_filled_percent_vec_.begin(), improve_order_filled_percent_vec_.end());

    supporting_order_filled_percent_ = HFSAT::VectorUtils::GetMedianAverage(supporting_order_filled_percent_vec_, true);
    best_level_order_filled_percent_ = HFSAT::VectorUtils::GetMedianAverage(best_level_order_filled_percent_vec_, true);
    aggressive_order_filled_percent_ = HFSAT::VectorUtils::GetMedianAverage(aggressive_order_filled_percent_vec_, true);
    improve_order_filled_percent_ = HFSAT::VectorUtils::GetMedianAverage(improve_order_filled_percent_vec_, true);
  }

  void ComputeStatistics() {
    std::vector<double> pnl_vec_;
    GetPNLVec(pnl_vec_);
    if (pnl_vec_.size() == 0) {
      return;
    }
    pnl_average_ = HFSAT::VectorUtils::GetWeightedMean(pnl_vec_, weights_vec_);
    pnl_stdev_ = HFSAT::VectorUtils::GetWeightedStdev(pnl_vec_, weights_vec_);
    pnl_skewness_ = HFSAT::VectorUtils::GetWeightedSkewness(pnl_vec_, pnl_average_, pnl_stdev_, weights_vec_);
    pnl_sharpe_ = (pnl_stdev_ > 0) ? (pnl_average_ / pnl_stdev_) : (0);

    // sorting once for the following operations
    sort(pnl_vec_.begin(), pnl_vec_.end());
    pnl_conservative_average_ = HFSAT::VectorUtils::GetWeightedConservativeAverageFifth(pnl_vec_, weights_vec_);
    pnl_median_average_ = HFSAT::VectorUtils::GetWeightedMedianAverage(pnl_vec_, true, weights_vec_);

    std::vector<double> min_pnl_vec_;
    GetMinPNLVec(min_pnl_vec_);

    std::vector<size_t> min_pnl_sort_idx_(min_pnl_vec_.size());
    for (size_t i = 0; i != min_pnl_sort_idx_.size(); ++i) min_pnl_sort_idx_[i] = i;

    std::sort(min_pnl_sort_idx_.begin(), min_pnl_sort_idx_.end(),
              [&min_pnl_vec_](size_t i1, size_t i2) { return min_pnl_vec_[i1] < min_pnl_vec_[i2]; });

    // sorting once for the following operationsi
    if (!min_pnl_vec_.empty()) {
      if (min_pnl_vec_.size() == weights_vec_.size()) {
        double sum_weights_ = 0;
        for (size_t i = 0; i != min_pnl_sort_idx_.size(); ++i) sum_weights_ += weights_vec_[min_pnl_sort_idx_[i]];

        double cumul_weights_ = 0;
        for (size_t i = 0; i != min_pnl_sort_idx_.size(); ++i) {
          if ((cumul_weights_ + weights_vec_[min_pnl_sort_idx_[i]]) > (sum_weights_ * 0.05)) {
            ninety_five_percentile_ = min_pnl_vec_[min_pnl_sort_idx_[i]];
            break;
          }
        }
      } else {
        ninety_five_percentile_ =
            min_pnl_vec_[min_pnl_sort_idx_[std::max(0, (int)(((double)min_pnl_vec_.size()) * 0.05))]];
      }
      if (ninety_five_percentile_ >= -1) {
        ninety_five_percentile_ = -1;
      }
      pnl_by_maxloss_ = pnl_average_ / -ninety_five_percentile_;
    }

    {
      // change this from end of day pnls to maxloss adjusted end of day pnls
      // where maxloss is computed above
      double weighted_min_pnl_ = min_pnl_vec_[min_pnl_sort_idx_[0]];
      if (min_pnl_vec_.size() == weights_vec_.size()) {
        weighted_min_pnl_ = weighted_min_pnl_ * weights_vec_[min_pnl_sort_idx_[0]];
      }
      double sum_losses_ =
          std::min(HFSAT::VectorUtils::GetWeightedSumLosses(pnl_vec_, weights_vec_), weighted_min_pnl_);
      if (sum_losses_ < 0) {
        gain_to_pain_ratio_ = HFSAT::VectorUtils::GetWeightedSum(pnl_vec_, weights_vec_) / -sum_losses_;
      } else {
        gain_to_pain_ratio_ = 10;  // a high enough positive number
      }
    }

    volume_average_ = GetAverageVolume();

    pnl_per_contract_ = (volume_average_ > 0) ? (pnl_average_ / volume_average_) : (-100);

    GetFillStats();

    average_abs_position_ = GetAverageAbsPosition();

    average_msg_count_ = GetAverageMsgCount();

    average_num_opentrade_hits_ = GetAverageOpenTradeHits();

    average_abs_closing_position_ = GetAverageAbsClosingPosition();

    average_unit_trade_size_ = GetAverageUnitTradeSize();

    median_average_time_to_close_trades_ = GetMedianTimeToCloseTrades();
    average_max_time_to_close_trades_ = GetAverageMaxTimeToCloseTrades();
    median_volume_normalized_average_time_to_close_trades_ = GetMedianNormalizedTTC();
    average_pnl_zscore_ = GetAveragePNLZscore();
    average_min_adjusted_pnl_ = GetAverageMinAdjustedPNL();
    pnl_sign_bias_ = GetPnlSignBias();

    percent_positive_trades_ = GetPercentPositiveTrades();

    average_max_drawdown_ = GetAverageMaxDrawdown();

    max_cumulative_dd_ = GetMaxCumulativeDrawdown();
    pnl_by_max_cumulative_dd_ = pnl_average_ / max_cumulative_dd_;
    cumulative_pnl_ = GetCumulativePnl();
    cumulative_pnl_by_max_dd_ = double(cumulative_pnl_) / double(max_cumulative_dd_);
    longest_negative_stretch_ = GetLongestStretch();

    if (average_max_drawdown_ > 0) {
      dd_adj_pnl_average_ = pnl_average_ / average_max_drawdown_;
      sqrt_dd_adj_pnl_average_ = pnl_average_ / sqrt(average_max_drawdown_);

      if (median_average_time_to_close_trades_ > 0) {
        sqrt_dd_ttc_adj_pnl_average_ = sqrt_dd_adj_pnl_average_ / median_average_time_to_close_trades_;
        sqrt_dd_sqrt_ttc_adj_pnl_average_ = sqrt_dd_adj_pnl_average_ / sqrt(median_average_time_to_close_trades_);
      } else {
        sqrt_dd_ttc_adj_pnl_average_ = 0.00;
        sqrt_dd_sqrt_ttc_adj_pnl_average_ = 0.00;
      }
    } else {
      dd_adj_pnl_average_ = 0.00;
      sqrt_dd_adj_pnl_average_ = 0.00;
      sqrt_dd_ttc_adj_pnl_average_ = 0.00;
      sqrt_dd_sqrt_ttc_adj_pnl_average_ = 0.00;
    }
  }

  void addWeightsfromMap(std::map<int, double>& date_to_weight_map_) {
    weights_vec_.resize(result_line_vec_.size(), 0);
    for (unsigned int index_ = 0; index_ < result_line_vec_.size(); index_++) {
      if (date_to_weight_map_.find(result_line_vec_[index_].yyyymmdd_) != date_to_weight_map_.end()) {
        weights_vec_[index_] = date_to_weight_map_[result_line_vec_[index_].yyyymmdd_];
      } else {
        weights_vec_[index_] = 0;
      }
    }
  }
};

typedef std::vector<ResultLineSet> ResultLineSetVec;
typedef std::vector<ResultLineSet>::iterator ResultLineSetVecIter_t;
typedef std::map<std::string, ResultLineSet> FileToResultLineSetMap;
typedef std::map<std::string, ResultLineSet>::iterator FileToResultLineSetMapIter_t;
typedef std::map<std::string, ResultLineSet>::const_iterator FileToResultLineSetMapCIter_t;

inline bool SortResultLineSetPnlAverage(const ResultLineSet& d1, const ResultLineSet& d2) {
  return d1.pnl_average_ < d2.pnl_average_;
}
inline bool SortResultLineSetGainToPainRatio(const ResultLineSet& d1, const ResultLineSet& d2) {
  return d1.gain_to_pain_ratio_ < d2.gain_to_pain_ratio_;
}
inline bool SortResultLineSetPnlByMaxLoss(const ResultLineSet& d1, const ResultLineSet& d2) {
  return d1.pnl_by_maxloss_ < d2.pnl_by_maxloss_;
}

inline bool SortResultLineSetPnlSharpe(const ResultLineSet& d1, const ResultLineSet& d2) {
  return d1.pnl_sharpe_ < d2.pnl_sharpe_;
}
inline bool SortResultLineSetPnlConservativeAverage(const ResultLineSet& d1, const ResultLineSet& d2) {
  return d1.pnl_conservative_average_ < d2.pnl_conservative_average_;
}
inline bool SortResultLineSetPnlMedianAverage(const ResultLineSet& d1, const ResultLineSet& d2) {
  return d1.pnl_median_average_ < d2.pnl_median_average_;
}
#define SUB_STD_FACTOR 0.30
inline bool SortResultLineSetPnlAdjAverage(const ResultLineSet& d1, const ResultLineSet& d2) {
  return ((d1.pnl_average_ - SUB_STD_FACTOR * d1.pnl_stdev_) < (d2.pnl_average_ - SUB_STD_FACTOR * d2.pnl_stdev_));
}
#undef SUB_STD_FACTOR
inline bool SortResultLineSetDDPnl(const ResultLineSet& d1, const ResultLineSet& d2) {
  return d1.dd_adj_pnl_average_ < d2.dd_adj_pnl_average_;
}
inline bool SortResultLineSetDDPnlSqVolume(const ResultLineSet& d1, const ResultLineSet& d2) {
  return (d1.dd_adj_pnl_average_ * sqrt(d1.volume_average_)) < (d2.dd_adj_pnl_average_ * sqrt(d2.volume_average_));
}
inline bool SortResultLineSetSqDDPnlSqVolume(const ResultLineSet& d1, const ResultLineSet& d2) {
  return (d1.sqrt_dd_adj_pnl_average_ * sqrt(d1.volume_average_)) <
         (d2.sqrt_dd_adj_pnl_average_ * sqrt(d2.volume_average_));
}
inline bool SortResultLineSetSqDDTTCAdjPnl(const ResultLineSet& d1, const ResultLineSet& d2) {
  return (d1.sqrt_dd_ttc_adj_pnl_average_) < (d2.sqrt_dd_ttc_adj_pnl_average_);
}
inline bool SortResultLineSetSqDDSqTTCAdjPnl(const ResultLineSet& d1, const ResultLineSet& d2) {
  return (d1.sqrt_dd_sqrt_ttc_adj_pnl_average_) < (d2.sqrt_dd_sqrt_ttc_adj_pnl_average_);
}
inline bool SortResultLineSetMinAdjPnlAverage(const ResultLineSet& d1, const ResultLineSet& d2) {
  return (d1.average_min_adjusted_pnl_) < (d2.average_min_adjusted_pnl_);
}
inline bool SortResultLineSetMinAdjPnlSqrtVolume(const ResultLineSet& d1, const ResultLineSet& d2) {
  return (d1.average_min_adjusted_pnl_ * sqrt(d1.volume_average_)) <
         (d2.average_min_adjusted_pnl_ * sqrt(d2.volume_average_));
}
inline bool SortResultLineSetSqrtPnl(const ResultLineSet& d1, const ResultLineSet& d2) {
  return SortResultLineSetPnlAverage(d1, d2);
}
inline bool SortResultLineSetPnlVol(const ResultLineSet& d1, const ResultLineSet& d2) {
  return d1.pnl_average_ * d1.volume_average_ < d2.pnl_average_ * d2.volume_average_;
}
inline bool SortResultLineSetSqrtPnlVol(const ResultLineSet& d1, const ResultLineSet& d2) {
  return SortResultLineSetPnlVol(d1, d2);
}
inline bool SortResultLineSetPnlDD(const ResultLineSet& d1, const ResultLineSet& d2) {
  return SortResultLineSetDDPnl(d1, d2);
}
inline bool SortResultLineSetPnlSqrtDD(const ResultLineSet& d1, const ResultLineSet& d2) {
  return d1.sqrt_dd_adj_pnl_average_ < d2.sqrt_dd_adj_pnl_average_;
}
inline bool SortResultLineSetPnlSqrtVolByDD(const ResultLineSet& d1, const ResultLineSet& d2) {
  float score1_ = d1.pnl_average_ > 0 ? d1.pnl_average_ * sqrt(fabs(d1.volume_average_ / d1.average_max_drawdown_))
                                      : d1.pnl_average_ / sqrt(fabs(d1.volume_average_ / d1.average_max_drawdown_));
  float score2_ = d2.pnl_average_ > 0 ? d2.pnl_average_ * sqrt(fabs(d2.volume_average_ / d2.average_max_drawdown_))
                                      : d2.pnl_average_ / sqrt(fabs(d2.volume_average_ / d2.average_max_drawdown_));
  return score1_ < score2_;
}
inline bool SortResultLineSetPnlSqrtVol(const ResultLineSet& d1, const ResultLineSet& d2) {
  float score1_ =
      d1.pnl_average_ > 0 ? d1.pnl_average_ * sqrt(d1.volume_average_) : d1.pnl_average_ / sqrt(d1.volume_average_);
  float score2_ =
      d2.pnl_average_ > 0 ? d2.pnl_average_ * sqrt(d2.volume_average_) : d2.pnl_average_ / sqrt(d2.volume_average_);
  return score1_ < score2_;
}
inline bool SortResultLineSetSqrtPnlVolByDD(const ResultLineSet& d1, const ResultLineSet& d2) {
  return sqrtSign(d1.dd_adj_pnl_average_ * d1.volume_average_);
}
inline bool SortResultLineSetSqrtPnlVolBySqrtDDTTC(const ResultLineSet& d1, const ResultLineSet& d2) {
  double dd_ttc_adjusted_vol_average_1 =
      sqrt(d1.volume_average_ / d1.average_max_drawdown_ * d1.median_average_time_to_close_trades_);
  double dd_ttc_adjusted_vol_average_2 =
      sqrt(d2.volume_average_ / d2.average_max_drawdown_ * d2.median_average_time_to_close_trades_);

  double score1_ = d1.pnl_average_ > 0 ? sqrt(d1.pnl_average_) * dd_ttc_adjusted_vol_average_1
                                       : -sqrt(-d1.pnl_average_) / dd_ttc_adjusted_vol_average_1;
  double score2_ = d2.pnl_average_ > 0 ? sqrt(d2.pnl_average_) * dd_ttc_adjusted_vol_average_2
                                       : -sqrt(-d2.pnl_average_) / dd_ttc_adjusted_vol_average_2;
  return score1_ < score2_;
}
inline bool SortResultLineSetSqrtPnlVolByTTCDD(const ResultLineSet& d1, const ResultLineSet& d2) {
  double dd_ttc_adjusted_vol_average_1 =
      sqrt(d1.volume_average_) / (d1.average_max_drawdown_ * d1.median_average_time_to_close_trades_);
  double dd_ttc_adjusted_vol_average_2 =
      sqrt(d2.volume_average_) / (d2.average_max_drawdown_ * d2.median_average_time_to_close_trades_);

  double score1_ = d1.pnl_average_ > 0 ? sqrt(d1.pnl_average_) * dd_ttc_adjusted_vol_average_1
                                       : -sqrt(-d1.pnl_average_) / dd_ttc_adjusted_vol_average_1;
  double score2_ = d2.pnl_average_ > 0 ? sqrt(d2.pnl_average_) * dd_ttc_adjusted_vol_average_2
                                       : -sqrt(-d2.pnl_average_) / dd_ttc_adjusted_vol_average_2;
  return score1_ < score2_;
}
inline bool SortResultLineSetSqrtPnlSharpe(const ResultLineSet& d1, const ResultLineSet& d2) {
  return SortResultLineSetPnlSharpe(d1, d2);
}
inline bool SortResultLineSetPnlSharpeVol(const ResultLineSet& d1, const ResultLineSet& d2) {
  return d1.pnl_sharpe_ * d1.volume_average_ < d2.pnl_sharpe_ * d2.volume_average_;
}
inline bool SortResultLineSetPnlSharpeByDD(const ResultLineSet& d1, const ResultLineSet& d2) {
  return d1.pnl_sharpe_ / fabs(d1.average_max_drawdown_) < d2.pnl_sharpe_ / fabs(d2.average_max_drawdown_);
}
inline bool SortResultLineSetSqrtPnlByDD(const ResultLineSet& d1, const ResultLineSet& d2) {
  return SortResultLineSetPnlSharpeByDD(d1, d2);
}
inline bool SortResultLineSetSqrtPnlSharpeVolByDD(const ResultLineSet& d1, const ResultLineSet& d2) {
  return d1.dd_adj_pnl_average_ * d1.volume_average_ / d1.pnl_stdev_ <
         d1.dd_adj_pnl_average_ * d2.volume_average_ / d1.pnl_stdev_;
}
inline bool SortResultLineSetSqrtPnlSharpeVolBySqrtDD(const ResultLineSet& d1, const ResultLineSet& d2) {
  return SortResultLineSetSqrtPnlSharpeVolByDD(d1, d2);
}
inline bool SortResultLineSetSqrtPnlSharpeVolByDDByTTC(const ResultLineSet& d1, const ResultLineSet& d2) {
  return (sqrtSign(d1.pnl_sharpe_ * d1.volume_average_) /
          (d1.average_max_drawdown_ * d1.median_average_time_to_close_trades_)) <
         (sqrtSign(d2.pnl_sharpe_ * d2.volume_average_) /
          (d2.average_max_drawdown_ * d2.median_average_time_to_close_trades_));
}
inline bool SortResultLineSetSqrtPnlSharpeVolBySqrtDDTTC(const ResultLineSet& d1, const ResultLineSet& d2) {
  return (d1.sqrt_dd_sqrt_ttc_adj_pnl_average_ * sqrt(d1.volume_average_ / d1.pnl_stdev_)) <
         (d2.sqrt_dd_sqrt_ttc_adj_pnl_average_ * sqrt(d2.volume_average_ / d2.pnl_stdev_));
}
inline bool SortResultLineSetPnlByMaxCumulativeDD(const ResultLineSet& d1, const ResultLineSet& d2) {
  return d1.pnl_by_max_cumulative_dd_ < d2.pnl_by_max_cumulative_dd_;
}
inline bool SortResultLineSetInvSqrtPnlAbsPos(const ResultLineSet& d1, const ResultLineSet& d2) {
  // Note this counter-intuitive sort algo
  // ranks higher risk strategies over lower risk strategies.
  // I only ever use it when forcing higher-risk trading styles.

  if (d1.pnl_average_ * d2.pnl_average_ <= 0) {  // one -ve / zero , one +ve / zero
    return (d1.pnl_average_ < d2.pnl_average_);
  } else if (d1.pnl_average_ < 0) {  // both -ve , reward one with better ( pnl / abs-pos )
    return (-sqrt(fabs(d1.pnl_average_)) / d1.average_abs_position_) <
           (-sqrt(fabs(d2.pnl_average_)) / d2.average_abs_position_);
  }

  // Both +ve , reward one with better pnl * abs-pos
  return (sqrt(d1.pnl_average_) * d1.average_abs_position_) < (sqrt(d2.pnl_average_) * d2.average_abs_position_);
}

inline bool SortResultLineSetInvSqrtPnlAbsPosSqrtTTC(const ResultLineSet& d1, const ResultLineSet& d2) {
  // Note this counter-intuitive sort algo
  // ranks higher risk strategies over lower risk strategies.
  // I only ever use it when forcing higher-risk trading styles.

  if (d1.pnl_average_ * d2.pnl_average_ <= 0) {  // one -ve / zero , one +ve / zero
    return (d1.pnl_average_ < d2.pnl_average_);
  } else if (d1.pnl_average_ < 0) {  // both -ve , reward one with better ( pnl / abs-pos )
    return (-sqrt(fabs(d1.pnl_average_)) / (d1.average_abs_position_ * sqrt(d1.median_average_time_to_close_trades_))) <
           (-sqrt(fabs(d2.pnl_average_)) / (d2.average_abs_position_ * sqrt(d2.median_average_time_to_close_trades_)));
  }

  // Both +ve , reward one with better pnl * abs-pos
  return (sqrt(d1.pnl_average_) * d1.average_abs_position_ * sqrt(d1.median_average_time_to_close_trades_)) <
         (sqrt(d2.pnl_average_) * d2.average_abs_position_ * sqrt(d2.median_average_time_to_close_trades_));
}
inline bool SortResultLineSetPnlSharpeAverage(const ResultLineSet& d1, const ResultLineSet& d2) {
  return ((d1.pnl_stdev_ == 0 || d2.pnl_stdev_ == 0)
              ? (d1.pnl_average_ < d2.pnl_average_)
              : ((d1.pnl_average_ * fabs(d1.pnl_sharpe_)) < (d2.pnl_average_ * fabs(d2.pnl_sharpe_))));
}
inline bool SortResultLineSetPnlSignAverage(const ResultLineSet& d1, const ResultLineSet& d2) {
  return ((d1.pnl_average_ * d1.pnl_sign_bias_) < (d2.pnl_average_ * d2.pnl_sign_bias_));
}

typedef enum {
  kCNAPnlConservativeAverage,
  kCNAPnlAverage,
  kCNAGainPainRatio,
  kCNAPnlByMaxloss,
  kCNAPnlSharpe,
  kCNAPnlMedianAverage,
  kCNAPnlAdjAverage,
  kCNADDAdjPnlAverage,
  kCNADDAdjPnlSqrtVolume,
  kCNASqDDAdjPnlSqrtVolume,
  kCNASqDDTTCAdjPnl,
  kCNASqDDSqTTCAdjPnl,
  kCNAMinAdjPnlAverage,
  kCNAMinAdjPnlSqrtVolume,
  kCNASqrtPnl,
  kCNAPnlVol,
  kCNASqrtPnlVol,
  kCNAPnlDD,
  kCNAPnlSqrtDD,
  kCNAPnlSqrtVolByDD,
  kCNASqrtPnlVolByDD,
  kCNASqrtPnlVolBySqrtDDTTC,
  kCNASqrtPnlVolByTTCDD,
  kCNASqrtPnlSharpe,
  kCNAPnlSharpeVol,
  kCNASqrtPnlSqrtVol,
  kCNAPnlSharpeByDD,
  kCNASqrtPnlByDD,
  kCNAPnlByMaxCumulativeDD,
  kCNASqrtPnlSharpeVolByDD,
  kCNASqrtPnlSharpeVolBySqrtDD,
  kCNASqrtPnlSharpeVolByDDByTTC,
  kCNASqrtPnlSharpeVolBySqrtDDTTC,
  kCNAInvSqrtPnlAbsPos,
  kCNAInvSqrtPnlAbsPosSqrtTTC,
  kCNAPnlSharpeAverage,
  kCNAPnlSignAverage,
  kCNAPnlSqrtVol,
  kCNAMAX
} ChooseNextAlgo_t;

inline ChooseNextAlgo_t GetChooseNextAlgoFromString(const char* algo_string_) {
  ChooseNextAlgo_t cna_ = kCNADDAdjPnlSqrtVolume;
  if (strcmp(algo_string_, "kCNAPnlConservativeAverage") == 0) {
    cna_ = kCNAPnlConservativeAverage;
  }
  if (strcmp(algo_string_, "kCNAPnlAverage") == 0) {
    cna_ = kCNAPnlAverage;
  }
  if (strcmp(algo_string_, "kCNAGainPainRatio") == 0) {
    cna_ = kCNAGainPainRatio;
  }
  if (strcmp(algo_string_, "kCNAPnlByMaxloss") == 0) {
    cna_ = kCNAPnlByMaxloss;
  }
  if (strcmp(algo_string_, "kCNAPnlSharpe") == 0) {
    cna_ = kCNAPnlSharpe;
  }
  if (strcmp(algo_string_, "kCNAPnlMedianAverage") == 0) {
    cna_ = kCNAPnlMedianAverage;
  }
  if (strcmp(algo_string_, "kCNAPnlAdjAverage") == 0) {
    cna_ = kCNAPnlAdjAverage;
  }
  if (strcmp(algo_string_, "kCNADDAdjPnlAverage") == 0) {
    cna_ = kCNADDAdjPnlAverage;
  }
  if (strcmp(algo_string_, "kCNADDAdjPnlSqrtVolume") == 0) {
    cna_ = kCNADDAdjPnlSqrtVolume;
  }
  if (strcmp(algo_string_, "kCNASqDDAdjPnlSqrtVolume") == 0) {
    cna_ = kCNASqDDAdjPnlSqrtVolume;
  }
  if (strcmp(algo_string_, "kCNASqDDTTCAdjPnl") == 0) {
    cna_ = kCNASqDDTTCAdjPnl;
  }
  if (strcmp(algo_string_, "kCNASqDDSqTTCAdjPnl") == 0) {
    cna_ = kCNASqDDSqTTCAdjPnl;
  }
  if (strcmp(algo_string_, "kCNAMinAdjPnlAverage") == 0) {
    cna_ = kCNAMinAdjPnlAverage;
  }
  if (strcmp(algo_string_, "kCNAMinAdjPnlSqrtVolume") == 0) {
    cna_ = kCNAMinAdjPnlSqrtVolume;
  }
  if (strcmp(algo_string_, "kCNASqrtPnl") == 0) {
    cna_ = kCNASqrtPnl;
  }
  if (strcmp(algo_string_, "kCNAPnlVol") == 0) {
    cna_ = kCNAPnlVol;
  }
  if (strcmp(algo_string_, "kCNASqrtPnlSqrtVol") == 0) {
    cna_ = kCNASqrtPnlSqrtVol;
  }
  if (strcmp(algo_string_, "kCNAPnlDD") == 0) {
    cna_ = kCNAPnlDD;
  }
  if (strcmp(algo_string_, "kCNAPnlSqrtDD") == 0) {
    cna_ = kCNAPnlSqrtDD;
  }
  if (strcmp(algo_string_, "kCNAPnlSqrtVolByDD") == 0) {
    cna_ = kCNAPnlSqrtVolByDD;
  }
  if (strcmp(algo_string_, "kCNASqrtPnlVolByDD") == 0) {
    cna_ = kCNASqrtPnlVolByDD;
  }
  if (strcmp(algo_string_, "kCNASqrtPnlVolBySqrtDDTTC") == 0) {
    cna_ = kCNASqrtPnlVolBySqrtDDTTC;
  }
  if (strcmp(algo_string_, "kCNASqrtPnlVolByTTCDD") == 0) {
    cna_ = kCNASqrtPnlVolByTTCDD;
  }
  if (strcmp(algo_string_, "kCNAPnlSharpe") == 0) {
    cna_ = kCNAPnlSharpe;
  }
  if (strcmp(algo_string_, "kCNASqrtPnlSharpe") == 0) {
    cna_ = kCNASqrtPnlSharpe;
  }
  if (strcmp(algo_string_, "kCNAPnlSharpeVol") == 0) {
    cna_ = kCNAPnlSharpeVol;
  }
  if (strcmp(algo_string_, "kCNASqrtPnlVol") == 0) {
    cna_ = kCNASqrtPnlVol;
  }
  if (strcmp(algo_string_, "kCNAPnlByMaxCumulativeDD") == 0) {
    cna_ = kCNAPnlByMaxCumulativeDD;
  }
  if (strcmp(algo_string_, "kCNAPnlSharpeByDD") == 0) {
    cna_ = kCNAPnlSharpeByDD;
  }
  if (strcmp(algo_string_, "kCNASqrtPnlByDD") == 0) {
    cna_ = kCNASqrtPnlByDD;
  }
  if (strcmp(algo_string_, "kCNASqrtPnlSharpeVolByDD") == 0) {
    cna_ = kCNASqrtPnlSharpeVolByDD;
  }
  if (strcmp(algo_string_, "kCNASqrtPnlSharpeVolBySqrtDD") == 0) {
    cna_ = kCNASqrtPnlSharpeVolBySqrtDD;
  }
  if (strcmp(algo_string_, "kCNASqrtPnlSharpeVolByDDByTTC") == 0) {
    cna_ = kCNASqrtPnlSharpeVolByDDByTTC;
  }
  if (strcmp(algo_string_, "kCNASqrtPnlSharpeVolBySqrtDDTTC") == 0) {
    cna_ = kCNASqrtPnlSharpeVolBySqrtDDTTC;
  }
  if (strcmp(algo_string_, "kCNAInvSqrtPnlAbsPos") == 0) {
    cna_ = kCNAInvSqrtPnlAbsPos;
  }
  if (strcmp(algo_string_, "kCNAInvSqrtPnlAbsPosSqrtTTC") == 0) {
    cna_ = kCNAInvSqrtPnlAbsPosSqrtTTC;
  }
  if (strcmp(algo_string_, "kCNAPnlSharpeAverage") == 0) {
    cna_ = kCNAPnlSharpeAverage;
  }
  if (strcmp(algo_string_, "kCNAPnlSignAverage") == 0) {
    cna_ = kCNAPnlSignAverage;
  }
  if (strcmp(algo_string_, "kCNAPnlSqrtVol") == 0) {
    cna_ = kCNAPnlSqrtVol;
  }

  return cna_;
}

inline auto GetSortFunc(const int algo_) -> bool (*)(const ResultLineSet& d1, const ResultLineSet& d2) {
  switch (algo_) {
    case kCNAPnlConservativeAverage:
      return SortResultLineSetPnlConservativeAverage;
      break;
    case kCNAPnlAverage:
      return SortResultLineSetPnlAverage;
      break;
    case kCNAGainPainRatio:
      return SortResultLineSetGainToPainRatio;
      break;
    case kCNAPnlByMaxloss:
      return SortResultLineSetPnlByMaxLoss;
      break;
    case kCNAPnlSharpe:
      return SortResultLineSetPnlSharpe;
      break;
    case kCNAPnlMedianAverage:
      return SortResultLineSetPnlMedianAverage;
      break;
    case kCNAPnlAdjAverage:
      return SortResultLineSetPnlAdjAverage;
      break;
    case kCNADDAdjPnlAverage:
      return SortResultLineSetDDPnl;
      break;
    case kCNAMinAdjPnlAverage:
      return SortResultLineSetMinAdjPnlAverage;
      break;
    case kCNAMinAdjPnlSqrtVolume:
      return SortResultLineSetMinAdjPnlSqrtVolume;
      break;
    case kCNADDAdjPnlSqrtVolume:
      return SortResultLineSetDDPnlSqVolume;
      break;
    case kCNASqDDAdjPnlSqrtVolume:
      return SortResultLineSetSqDDPnlSqVolume;
      break;
    case kCNASqDDTTCAdjPnl:
      return SortResultLineSetSqDDTTCAdjPnl;
      break;
    case kCNASqDDSqTTCAdjPnl:
      return SortResultLineSetSqDDSqTTCAdjPnl;
      break;
    case kCNASqrtPnl:
      return SortResultLineSetSqrtPnl;
      break;
    case kCNAPnlVol:
      return SortResultLineSetPnlVol;
      break;
    case kCNASqrtPnlVol:
      return SortResultLineSetSqrtPnlVol;
      break;
    case kCNAPnlDD:
      return SortResultLineSetPnlDD;
      break;
    case kCNAPnlByMaxCumulativeDD:
      return SortResultLineSetPnlByMaxCumulativeDD;
      break;
    case kCNAPnlSqrtDD:
      return SortResultLineSetPnlSqrtDD;
      break;
    case kCNAPnlSqrtVolByDD:
      return SortResultLineSetPnlSqrtVolByDD;
      break;
    case kCNASqrtPnlVolByDD:
      return SortResultLineSetSqrtPnlVolByDD;
      break;
    case kCNASqrtPnlVolBySqrtDDTTC:
      return SortResultLineSetSqrtPnlVolBySqrtDDTTC;
      break;
    case kCNASqrtPnlVolByTTCDD:
      return SortResultLineSetSqrtPnlVolByTTCDD;
      break;
    case kCNAPnlSharpeVol:
      return SortResultLineSetPnlSharpeVol;
      break;
    case kCNAPnlSharpeByDD:
      return SortResultLineSetPnlSharpeByDD;
      break;
    case kCNASqrtPnlByDD:
      return SortResultLineSetSqrtPnlByDD;
      break;
    case kCNASqrtPnlSharpeVolByDD:
      return SortResultLineSetSqrtPnlSharpeVolByDD;
      break;
    case kCNASqrtPnlSharpeVolBySqrtDD:
      return SortResultLineSetSqrtPnlSharpeVolBySqrtDD;
      break;
    case kCNASqrtPnlSqrtVol:
      return SortResultLineSetSqrtPnlVol;
      break;
    case kCNASqrtPnlSharpeVolByDDByTTC:
      return SortResultLineSetSqrtPnlSharpeVolByDDByTTC;
      break;
    case kCNASqrtPnlSharpeVolBySqrtDDTTC:
      return SortResultLineSetSqrtPnlSharpeVolBySqrtDDTTC;
      break;
    case kCNAInvSqrtPnlAbsPos:
      return SortResultLineSetInvSqrtPnlAbsPos;
      break;
    case kCNAInvSqrtPnlAbsPosSqrtTTC:
      return SortResultLineSetInvSqrtPnlAbsPosSqrtTTC;
      break;
    case kCNAPnlSharpeAverage:
      return SortResultLineSetPnlSharpeAverage;
      break;
    case kCNAPnlSignAverage:
      return SortResultLineSetPnlSignAverage;
      break;
    case kCNAPnlSqrtVol:
      return SortResultLineSetPnlSqrtVol;
      break;
    default:
      return SortResultLineSetPnlAdjAverage;
      break;
  }
  return SortResultLineSetPnlAverage;
}

inline void ChooseNext(ResultLineSetVec& result_set_vec_left_to_choose_, int& num_files_left_to_choose_,
                       const int algo_, ResultLineSetVec& result_set_vec_chosen_) {
  if ((num_files_left_to_choose_ > 0) && (result_set_vec_left_to_choose_.size() > 0)) {
    switch (algo_) {
      case kCNAPnlConservativeAverage:
        std::sort(result_set_vec_left_to_choose_.begin(), result_set_vec_left_to_choose_.end(),
                  SortResultLineSetPnlConservativeAverage);
        break;
      case kCNAPnlAverage:
        std::sort(result_set_vec_left_to_choose_.begin(), result_set_vec_left_to_choose_.end(),
                  SortResultLineSetPnlAverage);
        break;
      case kCNAGainPainRatio:
        std::sort(result_set_vec_left_to_choose_.begin(), result_set_vec_left_to_choose_.end(),
                  SortResultLineSetGainToPainRatio);
        break;
      case kCNAPnlByMaxloss:
        std::sort(result_set_vec_left_to_choose_.begin(), result_set_vec_left_to_choose_.end(),
                  SortResultLineSetPnlByMaxLoss);
        break;
      case kCNAPnlSharpe:
        std::sort(result_set_vec_left_to_choose_.begin(), result_set_vec_left_to_choose_.end(),
                  SortResultLineSetPnlSharpe);
        break;
      case kCNAPnlMedianAverage:
        std::sort(result_set_vec_left_to_choose_.begin(), result_set_vec_left_to_choose_.end(),
                  SortResultLineSetPnlMedianAverage);
        break;
      case kCNAPnlAdjAverage:
        std::sort(result_set_vec_left_to_choose_.begin(), result_set_vec_left_to_choose_.end(),
                  SortResultLineSetPnlAdjAverage);
        break;
      case kCNADDAdjPnlAverage:
        std::sort(result_set_vec_left_to_choose_.begin(), result_set_vec_left_to_choose_.end(), SortResultLineSetDDPnl);
        break;
      case kCNAMinAdjPnlAverage:
        std::sort(result_set_vec_left_to_choose_.begin(), result_set_vec_left_to_choose_.end(),
                  SortResultLineSetMinAdjPnlAverage);
        break;
      case kCNAMinAdjPnlSqrtVolume:
        std::sort(result_set_vec_left_to_choose_.begin(), result_set_vec_left_to_choose_.end(),
                  SortResultLineSetMinAdjPnlSqrtVolume);
        break;
      case kCNADDAdjPnlSqrtVolume:
        std::sort(result_set_vec_left_to_choose_.begin(), result_set_vec_left_to_choose_.end(),
                  SortResultLineSetDDPnlSqVolume);
        break;
      case kCNASqDDAdjPnlSqrtVolume:
        std::sort(result_set_vec_left_to_choose_.begin(), result_set_vec_left_to_choose_.end(),
                  SortResultLineSetSqDDPnlSqVolume);
        break;
      case kCNASqDDTTCAdjPnl:
        std::sort(result_set_vec_left_to_choose_.begin(), result_set_vec_left_to_choose_.end(),
                  SortResultLineSetSqDDTTCAdjPnl);
        break;
      case kCNASqDDSqTTCAdjPnl:
        std::sort(result_set_vec_left_to_choose_.begin(), result_set_vec_left_to_choose_.end(),
                  SortResultLineSetSqDDSqTTCAdjPnl);
        break;
      case kCNASqrtPnl:
        std::sort(result_set_vec_left_to_choose_.begin(), result_set_vec_left_to_choose_.end(),
                  SortResultLineSetSqrtPnl);
        break;
      case kCNAPnlVol:
        std::sort(result_set_vec_left_to_choose_.begin(), result_set_vec_left_to_choose_.end(),
                  SortResultLineSetPnlVol);
        break;
      case kCNASqrtPnlVol:
        std::sort(result_set_vec_left_to_choose_.begin(), result_set_vec_left_to_choose_.end(),
                  SortResultLineSetSqrtPnlVol);
        break;
      case kCNAPnlDD:
        std::sort(result_set_vec_left_to_choose_.begin(), result_set_vec_left_to_choose_.end(), SortResultLineSetPnlDD);
        break;
      case kCNAPnlByMaxCumulativeDD:
        std::sort(result_set_vec_left_to_choose_.begin(), result_set_vec_left_to_choose_.end(),
                  SortResultLineSetPnlByMaxCumulativeDD);
        break;
      case kCNAPnlSqrtDD:
        std::sort(result_set_vec_left_to_choose_.begin(), result_set_vec_left_to_choose_.end(),
                  SortResultLineSetPnlSqrtDD);
        break;
      case kCNAPnlSqrtVolByDD:
        std::sort(result_set_vec_left_to_choose_.begin(), result_set_vec_left_to_choose_.end(),
                  SortResultLineSetPnlSqrtVolByDD);
        break;
      case kCNASqrtPnlVolByDD:
        std::sort(result_set_vec_left_to_choose_.begin(), result_set_vec_left_to_choose_.end(),
                  SortResultLineSetSqrtPnlVolByDD);
        break;
      case kCNASqrtPnlVolBySqrtDDTTC:
        std::sort(result_set_vec_left_to_choose_.begin(), result_set_vec_left_to_choose_.end(),
                  SortResultLineSetSqrtPnlVolBySqrtDDTTC);
        break;
      case kCNASqrtPnlVolByTTCDD:
        std::sort(result_set_vec_left_to_choose_.begin(), result_set_vec_left_to_choose_.end(),
                  SortResultLineSetSqrtPnlVolByTTCDD);
        break;
      case kCNAPnlSharpeVol:
        std::sort(result_set_vec_left_to_choose_.begin(), result_set_vec_left_to_choose_.end(),
                  SortResultLineSetPnlSharpeVol);
        break;
      case kCNAPnlSharpeByDD:
        std::sort(result_set_vec_left_to_choose_.begin(), result_set_vec_left_to_choose_.end(),
                  SortResultLineSetPnlSharpeByDD);
        break;
      case kCNASqrtPnlByDD:
        std::sort(result_set_vec_left_to_choose_.begin(), result_set_vec_left_to_choose_.end(),
                  SortResultLineSetSqrtPnlByDD);
        break;
      case kCNASqrtPnlSharpeVolByDD:
        std::sort(result_set_vec_left_to_choose_.begin(), result_set_vec_left_to_choose_.end(),
                  SortResultLineSetSqrtPnlSharpeVolByDD);
        break;
      case kCNASqrtPnlSharpeVolBySqrtDD:
        std::sort(result_set_vec_left_to_choose_.begin(), result_set_vec_left_to_choose_.end(),
                  SortResultLineSetSqrtPnlSharpeVolBySqrtDD);
        break;
      case kCNASqrtPnlSqrtVol:
        std::sort(result_set_vec_left_to_choose_.begin(), result_set_vec_left_to_choose_.end(),
                  SortResultLineSetSqrtPnlVol);
        break;
      case kCNASqrtPnlSharpeVolByDDByTTC:
        std::sort(result_set_vec_left_to_choose_.begin(), result_set_vec_left_to_choose_.end(),
                  SortResultLineSetSqrtPnlSharpeVolByDDByTTC);
        break;
      case kCNASqrtPnlSharpeVolBySqrtDDTTC:
        std::sort(result_set_vec_left_to_choose_.begin(), result_set_vec_left_to_choose_.end(),
                  SortResultLineSetSqrtPnlSharpeVolBySqrtDDTTC);
        break;
      case kCNAInvSqrtPnlAbsPos:
        std::sort(result_set_vec_left_to_choose_.begin(), result_set_vec_left_to_choose_.end(),
                  SortResultLineSetInvSqrtPnlAbsPos);
        break;
      case kCNAInvSqrtPnlAbsPosSqrtTTC:
        std::sort(result_set_vec_left_to_choose_.begin(), result_set_vec_left_to_choose_.end(),
                  SortResultLineSetInvSqrtPnlAbsPosSqrtTTC);
        break;
      case kCNAPnlSharpeAverage:
        std::sort(result_set_vec_left_to_choose_.begin(), result_set_vec_left_to_choose_.end(),
                  SortResultLineSetPnlSharpeAverage);
        break;
      case kCNAPnlSignAverage:
        std::sort(result_set_vec_left_to_choose_.begin(), result_set_vec_left_to_choose_.end(),
                  SortResultLineSetPnlSignAverage);
        break;
      case kCNAPnlSqrtVol:
        std::sort(result_set_vec_left_to_choose_.begin(), result_set_vec_left_to_choose_.end(),
                  SortResultLineSetPnlSqrtVol);
        break;

      default:
        break;
    }
    result_set_vec_chosen_.push_back(result_set_vec_left_to_choose_.back());
    result_set_vec_left_to_choose_.pop_back();
    num_files_left_to_choose_--;
  }
}

inline void AddNextChoiceToSet(ResultLineSetVec& result_set_vec_left_to_choose_, int& num_files_left_to_choose_,
                               const int algo_, ResultLineSetVec& result_set_vec_chosen_,
                               ResultLineSet& sum_of_chosen_result_sets_) {
  if ((num_files_left_to_choose_ > 0) && (result_set_vec_left_to_choose_.size() > 0)) {
    ResultLineSet best_sum_of_chosen_result_sets_ = sum_of_chosen_result_sets_;

    best_sum_of_chosen_result_sets_.AddResultSet(result_set_vec_left_to_choose_[0]);
    ResultLineSet best_new_addition_ = result_set_vec_left_to_choose_[0];
    unsigned int best_index_ = 0u;

    auto SortFunc = GetSortFunc(algo_);
    for (unsigned int i = 1; i < result_set_vec_left_to_choose_.size(); i++) {
      ResultLineSet this_sum_of_chosen_result_sets_ = sum_of_chosen_result_sets_;
      this_sum_of_chosen_result_sets_.AddResultSet(result_set_vec_left_to_choose_[i]);

      if (SortFunc(best_sum_of_chosen_result_sets_, this_sum_of_chosen_result_sets_)) {  // current best is not
        best_sum_of_chosen_result_sets_ = this_sum_of_chosen_result_sets_;
        best_new_addition_ = result_set_vec_left_to_choose_[i];
        best_index_ = i;
      }
    }

    result_set_vec_chosen_.push_back(best_new_addition_);
    result_set_vec_left_to_choose_.erase(result_set_vec_left_to_choose_.begin() + best_index_);
    sum_of_chosen_result_sets_ = best_sum_of_chosen_result_sets_;
    num_files_left_to_choose_--;
  }
}

inline bool SanityCheckResultLine(ResultLine& _this_result_line_, std::string _shortcode_, int _unit_trade_size_) {
  int max_value_ = 500000;  // Better set this threshold specific to products
  if (_unit_trade_size_ > 0) {
    if (fabs(_this_result_line_.pnl_ / _unit_trade_size_) > max_value_) {
      return false;
    }
  } else {
    if (fabs(_this_result_line_.pnl_) > 20 * max_value_) {
      return false;
    }
  }
  return true;
  // Putting the sanity check for garbage pnl/volume
}

inline double GetScoreFromSortAlgo(const ResultLineSet& this_result_line_set_, ChooseNextAlgo_t cna_) {
  double this_score_ = 0;
  switch (cna_) {
    case kCNAPnlConservativeAverage:
      this_score_ = this_result_line_set_.pnl_conservative_average_;
      break;
    case kCNAPnlAverage:
      this_score_ = this_result_line_set_.pnl_average_;
      break;
    case kCNAGainPainRatio:
      this_score_ = this_result_line_set_.gain_to_pain_ratio_;
      break;
    case kCNAPnlSharpe:
      this_score_ = this_result_line_set_.pnl_sharpe_;
      break;
    case kCNAPnlMedianAverage:
      this_score_ = this_result_line_set_.pnl_median_average_;
      break;
    case kCNAPnlAdjAverage:
      this_score_ = this_result_line_set_.pnl_average_ - 0.30 * this_result_line_set_.pnl_stdev_;
      break;
    case kCNAPnlSharpeAverage:
      // because for 1 day summary stdev is 0, and for small periods like 2-3 days sharpe can be very high like >100
      // which dont make much sense for scoring
      this_score_ = (this_result_line_set_.pnl_stdev_ == 0
                         ? this_result_line_set_.pnl_average_
                         : this_result_line_set_.pnl_average_ * std::min(1.2, fabs(this_result_line_set_.pnl_sharpe_)));
      break;
    case kCNADDAdjPnlAverage:
      this_score_ = this_result_line_set_.dd_adj_pnl_average_;
      break;
    case kCNAMinAdjPnlAverage:
      this_score_ = this_result_line_set_.average_min_adjusted_pnl_;
      break;
    case kCNAMinAdjPnlSqrtVolume:
      this_score_ = this_result_line_set_.average_min_adjusted_pnl_ * sqrt(this_result_line_set_.volume_average_);
      break;
    case kCNADDAdjPnlSqrtVolume:
      this_score_ = this_result_line_set_.dd_adj_pnl_average_ * sqrt(this_result_line_set_.volume_average_);
      break;
    case kCNASqDDAdjPnlSqrtVolume:
      this_score_ = this_result_line_set_.sqrt_dd_adj_pnl_average_ * sqrt(this_result_line_set_.volume_average_);
      break;
    case kCNASqDDTTCAdjPnl:
      this_score_ = this_result_line_set_.sqrt_dd_ttc_adj_pnl_average_;
      break;
    case kCNASqDDSqTTCAdjPnl:
      this_score_ = this_result_line_set_.sqrt_dd_sqrt_ttc_adj_pnl_average_;
      break;
    case kCNASqrtPnl:
      this_score_ = this_result_line_set_.pnl_average_;
      break;
    case kCNAPnlVol:
      this_score_ = this_result_line_set_.pnl_average_ * this_result_line_set_.volume_average_;
      break;
    case kCNASqrtPnlVol:
      this_score_ = sqrtSign(this_result_line_set_.pnl_average_ * this_result_line_set_.volume_average_);
      break;
    case kCNAPnlDD:
      this_score_ = this_result_line_set_.dd_adj_pnl_average_;
      break;
    case kCNAPnlByMaxCumulativeDD:
      this_score_ = this_result_line_set_.pnl_by_max_cumulative_dd_;
      break;
    case kCNAPnlSqrtDD:
      this_score_ = this_result_line_set_.sqrt_dd_adj_pnl_average_;
      break;
    case kCNAPnlSqrtVolByDD: {
      if (this_result_line_set_.average_max_drawdown_ > 0) {
        float score_ =
            this_result_line_set_.pnl_average_ > 0
                ? this_result_line_set_.pnl_average_ *
                      sqrt(abs(this_result_line_set_.volume_average_ / this_result_line_set_.average_max_drawdown_))
                : this_result_line_set_.pnl_average_ /
                      sqrt(abs(this_result_line_set_.volume_average_ / this_result_line_set_.average_max_drawdown_));
        this_score_ = score_;
      }
      break;
    }
    case kCNASqrtPnlVolByDD: {
      this_score_ = sqrtSign(this_result_line_set_.dd_adj_pnl_average_ * this_result_line_set_.volume_average_);
      break;
    }
    case kCNASqrtPnlVolBySqrtDDTTC: {
      if (this_result_line_set_.average_max_drawdown_ > 0) {
        double dd_ttc_adjusted_vol_average_ =
            sqrt(this_result_line_set_.volume_average_ / this_result_line_set_.average_max_drawdown_ *
                 this_result_line_set_.median_average_time_to_close_trades_);
        double score_ = this_result_line_set_.pnl_average_ > 0
                            ? sqrt(this_result_line_set_.pnl_average_) * dd_ttc_adjusted_vol_average_
                            : -sqrt(-this_result_line_set_.pnl_average_) / dd_ttc_adjusted_vol_average_;
        this_score_ = score_;
      }
      break;
    }
    case kCNASqrtPnlVolByTTCDD: {
      if (this_result_line_set_.average_max_drawdown_ > 0) {
        double dd_ttc_adjusted_vol_average_ =
            sqrt(this_result_line_set_.volume_average_) /
            (this_result_line_set_.average_max_drawdown_ * this_result_line_set_.median_average_time_to_close_trades_);
        double score_ = this_result_line_set_.pnl_average_ > 0
                            ? sqrt(this_result_line_set_.pnl_average_) * dd_ttc_adjusted_vol_average_
                            : -sqrt(-this_result_line_set_.pnl_average_) / dd_ttc_adjusted_vol_average_;
        this_score_ = score_;
      }
      break;
    }
    case kCNAPnlSharpeVol:
      this_score_ = this_result_line_set_.pnl_sharpe_ * this_result_line_set_.volume_average_;
      break;
    case kCNAPnlSharpeByDD:
      this_score_ = this_result_line_set_.average_max_drawdown_ <= 0
                        ? 0
                        : this_result_line_set_.pnl_sharpe_ / abs(this_result_line_set_.average_max_drawdown_);
      break;
    case kCNASqrtPnlByDD:
      this_score_ =
          this_result_line_set_.average_max_drawdown_ <= 0
              ? 0
              : sqrtSign(this_result_line_set_.pnl_average_ / abs(this_result_line_set_.average_max_drawdown_));
      break;
    case kCNASqrtPnlSharpeVolByDD:
      this_score_ = this_result_line_set_.pnl_stdev_ <= 0 ? 0 : this_result_line_set_.dd_adj_pnl_average_ *
                                                                    this_result_line_set_.volume_average_ /
                                                                    this_result_line_set_.pnl_stdev_;
      break;
    case kCNASqrtPnlSharpeVolBySqrtDD:
      this_score_ = this_result_line_set_.pnl_stdev_ <= 0 ? 0 : this_result_line_set_.dd_adj_pnl_average_ *
                                                                    this_result_line_set_.volume_average_ /
                                                                    this_result_line_set_.pnl_stdev_;
      break;
    case kCNASqrtPnlSqrtVol:
      this_score_ = sqrtSign(this_result_line_set_.pnl_average_ * this_result_line_set_.volume_average_);
      break;
    case kCNASqrtPnlSharpeVolByDDByTTC:
      this_score_ = this_result_line_set_.average_max_drawdown_ <= 0
                        ? 0
                        : sqrtSign(this_result_line_set_.pnl_sharpe_ * this_result_line_set_.volume_average_) /
                              (this_result_line_set_.average_max_drawdown_ *
                               this_result_line_set_.median_average_time_to_close_trades_);
      break;
    case kCNASqrtPnlSharpeVolBySqrtDDTTC:
      this_score_ = this_result_line_set_.average_max_drawdown_ <= 0
                        ? 0
                        : sqrtSign(this_result_line_set_.pnl_sharpe_ * this_result_line_set_.volume_average_) /
                              sqrt(this_result_line_set_.average_max_drawdown_ *
                                   this_result_line_set_.median_average_time_to_close_trades_);
      break;
    case kCNAInvSqrtPnlAbsPos:
    case kCNAInvSqrtPnlAbsPosSqrtTTC:
    default:
      break;
  }

  return this_score_;
}
}
