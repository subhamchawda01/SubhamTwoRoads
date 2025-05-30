/**
    \file Indicators/book_info_manager.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#pragma once

#include <map>
#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "dvctrade/Indicators/slow_stdev_calculator.hpp"
#include "dvctrade/Indicators/pca_weights_manager.hpp"
#include "dvctrade/Indicators/stdev_ratio_calculator.hpp"

#define STDEV_RATIO_CAP 4.00

namespace HFSAT {

class BookInfoManager : public SecurityMarketViewChangeListener, public StdevRatioListener {
 protected:
  DebugLogger& dbglogger_;
  const Watch& watch_;

  SecurityMarketView& indep_market_view_;
  unsigned int max_num_levels_;

  double decay_page_factor_;

  bool computing_sum_size_;
  bool computing_sum_factor_size_;

  uint16_t modify_all_bits_;
  int last_spread_increments_;
  bool recompute_bid_factors_;
  bool recompute_ask_factors_;

  std::vector<int> bid_size_;
  std::vector<int> bid_ordercount_;
  std::vector<double> bid_price_;

  std::vector<int> ask_size_;
  std::vector<int> ask_ordercount_;
  std::vector<double> ask_price_;

  int last_decay_factor_update_msecs_;

 public:
  struct BookInfoStruct {
    StdevRatioCalculator* stdev_ratio_calculator_;
    double current_dep_stdev_ratio_;
    double stdev_duration_;

    unsigned int num_levels_;
    double decay_factor_;
    std::vector<double> decay_vector_;

    bool allow_scaling_;
    bool computing_sum_size_;
    bool computing_sum_factor_size_;
    bool computing_sum_price_;
    bool computing_sum_factor_price_size_;
    bool computing_sum_factor_order_;
    bool computing_sum_factor_price_order_;
    bool computing_sum_factor_;

    double sum_bid_size_;
    double sum_bid_factor_size_;
    double sum_bid_price_;
    double sum_bid_factor_price_size_;
    double sum_bid_factor_order_;
    double sum_bid_factor_price_order_;
    double sum_bid_factor_;

    double sum_ask_size_;
    double sum_ask_factor_size_;
    double sum_ask_price_;
    double sum_ask_factor_price_size_;
    double sum_ask_factor_order_;
    double sum_ask_factor_price_order_;
    double sum_ask_factor_;

    BookInfoStruct(unsigned int t_num_levels_, double t_decay_factor_, bool allow_scaling)
        : stdev_ratio_calculator_(nullptr),
          current_dep_stdev_ratio_(0),
          stdev_duration_(0),
          num_levels_(t_num_levels_),
          decay_factor_(t_decay_factor_),
          decay_vector_(std::max(24u, (2u * t_num_levels_ + 8u)), 0),  // not sure why it was fixed to 24 earlier
          allow_scaling_(allow_scaling) {
      if (allow_scaling) {
        // Can impact performance significantly if we are using it for prod with high number of events per sec
        // and number of levels are high, though its expected that high values of num_levels will be used with prods
        // which have holes etc in book and less num events
        decay_vector_.resize(std::max(50u, ((unsigned)(STDEV_RATIO_CAP + 1) * 2u * t_num_levels_ + 8u)), 0);
      }

      for (auto i = 0u; i < decay_vector_.size(); i++) {
        int fact = i;
        decay_vector_[i] = pow(sqrt(decay_factor_), (int)fact);
      }

      computing_sum_size_ = false;
      computing_sum_factor_size_ = false;
      computing_sum_price_ = false;
      computing_sum_factor_price_size_ = false;
      computing_sum_factor_order_ = false;
      computing_sum_factor_price_order_ = false;
      computing_sum_factor_ = false;

      sum_bid_size_ = 0;
      sum_bid_factor_size_ = 0;
      sum_bid_price_ = 0;
      sum_bid_factor_price_size_ = 0;
      sum_bid_factor_order_ = 0;
      sum_bid_factor_price_order_ = 0;
      sum_bid_factor_ = 0;

      sum_ask_size_ = 0;
      sum_ask_factor_size_ = 0;
      sum_ask_price_ = 0;
      sum_ask_factor_price_size_ = 0;
      sum_ask_factor_order_ = 0;
      sum_ask_factor_price_order_ = 0;
      sum_ask_factor_ = 0;
    }

    void ResetBidVariables() {
      if (computing_sum_size_) {
        sum_bid_size_ = 0;
      }

      if (computing_sum_factor_size_) {
        sum_bid_factor_size_ = 0;
      }

      if (computing_sum_price_) {
        sum_bid_price_ = 0;
      }

      if (computing_sum_factor_price_size_) {
        sum_bid_factor_price_size_ = 0;
      }

      if (computing_sum_factor_order_) {
        sum_bid_factor_order_ = 0;
      }

      if (computing_sum_factor_price_order_) {
        sum_bid_factor_price_order_ = 0;
      }

      if (computing_sum_factor_) {
        sum_bid_factor_ = 0;
      }
    }

    void ResetAskVariables() {
      if (computing_sum_size_) {
        sum_ask_size_ = 0;
      }

      if (computing_sum_factor_size_) {
        sum_ask_factor_size_ = 0;
      }

      if (computing_sum_price_) {
        sum_ask_price_ = 0;
      }

      if (computing_sum_factor_price_size_) {
        sum_ask_factor_price_size_ = 0;
      }

      if (computing_sum_factor_order_) {
        sum_ask_factor_order_ = 0;
      }

      if (computing_sum_factor_price_order_) {
        sum_ask_factor_price_order_ = 0;
      }

      if (computing_sum_factor_) {
        sum_ask_factor_ = 0;
      }
    }
  };

  //    std::map < std::pair < int, double >, BookInfoStruct * > book_info_structs_;
  //    std::map < int, BookInfoStruct * > book_info_structs_;
  std::vector<BookInfoStruct*> book_info_struct_;

 protected:
  BookInfoManager(DebugLogger& _dbglogger_, const Watch& _watch_, SecurityMarketView& _indep_market_view_);
  static std::map<std::string, BookInfoManager*> concise_indicator_description_map_;

 public:
  static BookInfoManager* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                            SecurityMarketView& _indep_market_view_);

  static void RemoveUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                   SecurityMarketView& _indep_market_view_);

  void InitializeValues();
  void InitializeDynamicValues(BookInfoStruct* book_info, double stdev_duration);

  inline void ResetLocalConstants(const unsigned int t_num_levels_);

  void ComputeSumSize(const unsigned int t_num_levels_, const double t_decay_factor_, const double stdev_duration = 0);
  void ComputeSumFactorSize(const unsigned int t_num_levels_, const double t_decay_factor_,
                            const double stdev_duration = 0);
  void ComputeSumPrice(const unsigned int t_num_levels_, const double t_decay_factor_, const double stdev_duration = 0);
  void ComputeSumFactorPriceSize(const unsigned int t_num_levels_, const double t_decay_factor_,
                                 const double stdev_duration = 0);
  void ComputeSumFactorOrder(const unsigned int t_num_levels_, const double t_decay_factor_,
                             const double stdev_duration = 0);
  void ComputeSumFactorPriceOrder(const unsigned int t_num_levels_, const double t_decay_factor_,
                                  const double stdev_duration = 0);
  void ComputeSumFactor(const unsigned int t_num_levels_, const double t_decay_factor_,
                        const double stdev_duration = 0);

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                    const MarketUpdateInfo& _market_update_info_);

  // functions
  static std::string VarName() { return "BookInfoManager"; }

  BookInfoStruct* GetBookInfoStruct(const unsigned int t_num_levels_, const double t_decay_factor_,
                                    double stdev_duration);

  void OnTimePeriodUpdate(const int num_pages_to_add);
  void OnStdevRatioUpdate(const unsigned int index_to_send, const double& r_new_scaled_volume_value);

 protected:
  inline void ComputeBidVariables(const MarketUpdateInfoLevelStruct& t_bid_level_, const unsigned int t_index_,
                                  const unsigned int t_int_price_level_);
  inline void ComputeAskVariables(const MarketUpdateInfoLevelStruct& t_bid_level_, const unsigned int t_index_,
                                  const unsigned int t_int_price_level_);
};
}
