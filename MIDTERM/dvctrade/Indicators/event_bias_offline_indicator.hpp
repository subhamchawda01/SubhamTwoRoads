/**
    \file Indicators/online_computed_negatively_correlated_pair.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_EVENT_BIAS_OFFLINE_H
#define BASE_INDICATORS_EVENT_BIAS_OFFLINE_H

#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_aflash_processor.hpp"

#define VAL_DECAY_MAX_LEN 600
#define VAL_EPSILON_INT 1e-2
#define DIFF_EPSILON_INT 1e-3

namespace HFSAT {

struct EBOIVals {
  std::map<uint8_t, double> actual_vals_;
  std::map<uint8_t, double> estimate_vals_;
  std::map<uint8_t, double> scale_beta_;
  uint16_t cat_id_;
  std::vector<uint8_t> cat_datum_;
  double mfm_received_;
  int event_mfm_;
  double act_margin_;
  double getflat_margin_;
};

class EventBiasOfflineIndicator : public CommonIndicator, public TimePeriodListener, public AflashListener {
 protected:
  SecurityMarketView &indep_market_view_;

  double event_pr_st_msecs_;  // the prior_price is computed as average price since event_pr_st_msecs_ prior to event
  int last_viewed_msecs_;     // last time the event_signal was updated

  /*   variables for computing the avgpx_pr_  */
  double moving_sumpx_pr_;
  double count_px_pr_;
  double avgpx_pr_;  // avg_prior_price

  double curr_px_;
  double pxch_pred_;
  bool is_active_;
  bool af_feeds_recv_;

  AF_MSGSPECS::Category *catg_;
  EBOIVals event_;
  double val_decay_factor_;
  std::vector<double> val_decay_vec_;

  double VAL_EPSILON;
  double DIFF_EPSILON;

  std::map<uint16_t, uint8_t> cat2release_;
  std::map<uint16_t, uint8_t> cat2revised_;

 public:
  static void CollectShortCodes(std::vector<std::string> &_shortcodes_affecting_this_indicator_,
                                std::vector<std::string> &_ors_source_needed_vec_,
                                const std::vector<const char *> &_tokens_);
  static EventBiasOfflineIndicator *GetUniqueInstance(DebugLogger &t_dbglogger_, const Watch &r_watch_,
                                                      const std::vector<const char *> &r_tokens_,
                                                      PriceType_t _basepx_pxtype_);
  static EventBiasOfflineIndicator *GetUniqueInstance(DebugLogger &_dbglogger_, const Watch &_watch_,
                                                      SecurityMarketView &_indep_market_view_, int _event_id_,
                                                      double _half_life_seconds_);

  EventBiasOfflineIndicator(DebugLogger &_dbglogger_, const Watch &_watch_,
                            const std::string &concise_indicator_description_, SecurityMarketView &_indep_market_view_,
                            int event_id_, double _half_life_seconds_);

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo &_market_update_info_);
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo &_trade_print_info_,
                           const MarketUpdateInfo &_market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}

  inline void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {}
  inline void OnMarketDataResumed(const unsigned int _security_id_) {}

  // watch listener interface
  void OnTimePeriodUpdate(const int num_pages_to_add_);

  // functions
  static std::string VarName() { return "EventBiasOfflineIndicator"; }
  bool GetReadinessRequired(const std::string &r_dep_shortcode_, const std::vector<const char *> &tokens_) const {
    return true;
  }

  void readEventsScale();
  void readEstimateEvents();
  void readMarginFile();

  void onAflashMsgNew(int uid_, timeval time_, char symbol_[4], uint8_t type_, uint8_t version_, uint8_t nfields_,
                      AFLASH_MDS::AFlashDatum fields[AFLASH_MDS::MAX_FIELDS], uint16_t category_id_);

 protected:
  std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems);
  void ProcessSignalUpdate(const int num_pages_to_add_);
};
}

#endif  // BASE_INDICATORS_EVENT_BIAS_H
