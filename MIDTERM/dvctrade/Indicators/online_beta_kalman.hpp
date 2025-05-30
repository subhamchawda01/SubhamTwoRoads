/*
 * online_beta_kalman.hpp
 *
 *  Created on: 15-Jan-2016
 *      Author: raghuram
 */

#ifndef DVCTRADE_INDICATORS_ONLINE_BETA_KALMAN_HPP_
#define DVCTRADE_INDICATORS_ONLINE_BETA_KALMAN_HPP_

/*
\file Indicators /
    online_beta_return.hpp
   \author : (c)Copyright Two Roads Technological Solutions Pvt Ltd 2011 Address : Suite No 351,
    Evoma, #14, Bhattarhalli, Old Madras Road, Near Garden City College, KR Puram, Bangalore 560049,
    India +
        91 80 4190 3551
*/

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/simple_returns.hpp"
#include "dvctrade/Indicators/simple_returns_port.hpp"
#include "dvctrade/Indicators/offline_returns_rlrdb.hpp"

namespace HFSAT {

/// Common interface extended by all classes listening to OnlineBetaKalman
/// to listen to changes in online computed correlation of the products
class OnlineBetaKalman : public IndicatorListener, public CommonIndicator, public TimePeriodListener {
 protected:
  // variables

  const SecurityMarketView &dep_market_view_;

  // computational variables

  CommonIndicator *indep_return_indicator_;
  SimpleReturns &dep_return_indicator_;

  const unsigned int return_history_msecs_;

  double init_var_;
  double beta_noise_;
  double return_noise_;

  OfflineReturnsRetLRDB &lrdb_;
  int last_lrinfo_updated_msecs_;
  LRInfo current_lrinfo_;
  double current_projection_multiplier_;

  // int last_new_page_msecs_;
  // int page_width_msecs_;

  bool dep_updated_;
  bool indep_updated_;

  // double decay_page_factor_;
  // std::vector<double> decay_vector_;
  // std::vector<double> decay_vector_sums_;
  // double inv_decay_sum_;

  // double dep_moving_avg_return_;
  // double indep_moving_avg_return_;
  // double moving_avg_squared_return_indep_;
  // double dep_indep_moving_avg_return_;
  // double min_unbiased_l2_norm_;
  std::string source_shortcode_;

  double gain_ = 0;
  double curr_var_ = 0;
  double prev_var_ = 0;
  double curr_beta_ = 0;

  double prev_beta_ = 0;

  std::string kalman_param_file_ = "";

  bool is_kalman_param_present_ = false;

  // moving avergae variables
  std::vector<double> curr_returns_vec_ = {0, 0};
  std::vector<double> last_returns_vec_ = {0, 0};
  std::vector<double> moving_avg_vec_ = {0, 0};

  int last_beta_msecs_ = 0;
  double current_correlation_ = 0;
  double upper_bound_;
  double lower_bound_;

  // functions
 public:
  static OnlineBetaKalman *GetUniqueInstance(DebugLogger &_dbglogger_, const Watch &_watch_,
                                             const SecurityMarketView &_dep_market_view_,
                                             std::string _source_shortcode_, const unsigned int t_return_history_msecs_,
                                             const double _init_var_, const double _beta_noise_,
                                             const double _return_noise_, PriceType_t _t_price_type_);

 protected:
  OnlineBetaKalman(DebugLogger &_dbglogger_, const Watch &_watch_, const std::string &concise_indicator_description_,
                   const SecurityMarketView &_dep_market_view_, std::string _source_shortcode_,
                   const unsigned int t_return_history_msecs_, const double _init_var_, const double _beta_noise_,
                   const double _return_noise_, PriceType_t _t_price_type_);

 public:
  ~OnlineBetaKalman() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo &_market_update_info_) {}
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo &_trade_print_info_,
                           const MarketUpdateInfo &_market_update_info_) {}
  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double _new_price_, double _old_price_, unsigned int is_data_interrupted_) {}

  inline void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  inline void OnMarketDataResumed(const unsigned int _security_id_);

  void OnIndicatorUpdate(const unsigned int &_indicator_index_, const double &_new_value_);

  inline void OnIndicatorUpdate(const unsigned int &_indicator_index_, const double &new_value_decrease_,
                                const double &new_value_nochange_, const double &new_value_increase_) {
    return;
  }

  void OnTimePeriodUpdate(const int num_pages_to_add_) { UpdateLRInfo(); }

  // functions
  static std::string VarName() { return "OnlineBetaKalman"; }

  static OnlineBetaKalman *GetUniqueInstance(DebugLogger &t_dbglogger_, const Watch &r_watch_,
                                             const std::vector<const char *> &r_tokens_, PriceType_t _basepx_pxtype_);
  static void CollectShortCodes(std::vector<std::string> &_shortcodes_affecting_this_indicator_,
                                std::vector<std::string> &_ors_source_needed_vec_,
                                const std::vector<const char *> &r_tokens_);

 protected:
  void InitializeValues();
  void UpdateLRInfo();
  inline void ComputeMultiplier() {
    current_projection_multiplier_ = current_lrinfo_.lr_coeff_;
    current_correlation_ = current_lrinfo_.lr_correlation_;
    if (current_projection_multiplier_ > 0 && current_correlation_ > 0) {
      if (fabs(current_correlation_) > 0.001) {
        upper_bound_ = current_projection_multiplier_ / current_correlation_;
        lower_bound_ = current_projection_multiplier_ * current_correlation_;
      }
    } else if (current_projection_multiplier_ < 0 && current_correlation_ < 0) {
      if (fabs(current_correlation_) > 0.001) {
        upper_bound_ = current_projection_multiplier_ * fabs(current_correlation_);
        lower_bound_ = current_projection_multiplier_ / fabs(current_correlation_);
      }
    }
  }
  void compute_beta();

  void get_kalman_params();
};
}

#endif
