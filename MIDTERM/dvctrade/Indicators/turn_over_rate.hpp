/**
    \file Indicators/turn_over_rate.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#pragma once

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/indicator_listener.hpp"
#include "dvctrade/Indicators/recent_simple_volume_measure.hpp"

namespace HFSAT {

// base listener class
class TurnOverRateListener {
 public:
  virtual ~TurnOverRateListener(){};
  virtual void OnTorUpdate(unsigned int t_index_, double t_new_tor_) = 0;
  // since this is only one dep based ( no need to have security_id as indentifier ), but index is used for multiple
  // period
};

// indexed listener class
class TurnOverRateListenerPair {
 public:
  unsigned int index__;
  TurnOverRateListener* tor_listener__;

 public:
  explicit TurnOverRateListenerPair(unsigned int _index_, TurnOverRateListener* _tor_listener__)
      : index__(_index_), tor_listener__(_tor_listener__) {}

  const TurnOverRateListenerPair& operator=(const TurnOverRateListenerPair& p) {
    index__ = p.index__;
    tor_listener__ = p.tor_listener__;
    return *this;
  }

  const bool operator==(const TurnOverRateListenerPair& p) {
    return (index__ == p.index__ && tor_listener__ == p.tor_listener__);
  }

  virtual ~TurnOverRateListenerPair() {}

  inline void OnTorUpdate(double _tor_value_) { tor_listener__->OnTorUpdate(index__, _tor_value_); }
};

// actual class
class TurnOverRate : public CommonIndicator, public RecentSimpleVolumeListener {
 protected:
  const SecurityMarketView& dep_market_view_;
  RecentSimpleVolumeMeasure* const p_recent_simple_volume_measure_;

  double t_secs_;
  bool is_ready_ = false;

  double moving_avg_sup_;
  double last_supply_;
  double current_supply_;

  std::vector<TurnOverRateListenerPair> tor_listener_pairs_;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& r_shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& r_ors_source_needed_vec_,
                                const std::vector<const char*>& r_tokens_);

  static TurnOverRate* GetUniqueInstance(DebugLogger& r_dbglogger_, const Watch& r_watch_,
                                         const std::vector<const char*>& r_tokens_, PriceType_t t_basepx_pxtype_);

  static TurnOverRate* GetUniqueInstance(DebugLogger& r_dbglogger_, const Watch& r_watch_,
                                         const SecurityMarketView& r_dep_market_view_, double t_fractional_secs_);

 protected:
  TurnOverRate(DebugLogger& r_dbglogger_, const Watch& r_watch_, const std::string& r_concise_indicator_description_,
               const SecurityMarketView& r_dep_market_view_, double t_fractional_secs_);

 public:
  ~TurnOverRate(){};

  // listener interface
  void OnMarketUpdate(const unsigned int t_security_id_, const MarketUpdateInfo& r_market_update_info_);

  inline void OnTradePrint(const unsigned int t_security_id_, const TradePrintInfo& r_trade_print_info_,
                           const MarketUpdateInfo& r_market_update_info_) {
    OnMarketUpdate(t_security_id_, r_market_update_info_);
  }

  inline void OnPortfolioPriceChange(double t_new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

  // functions
  static std::string VarName() { return "TurnOverRate"; }
  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);
  void OnVolumeUpdate(unsigned int t_index_, double t_vol_);

  inline void AddTurnOverRateListener(unsigned int _index_, TurnOverRateListener* _new_listener_) {
    if (_new_listener_ != NULL) {
      TurnOverRateListenerPair _new_tor_pair_(_index_, _new_listener_);
      VectorUtils::UniqueVectorAdd(tor_listener_pairs_, _new_tor_pair_);
    }
  }

 protected:
  void InitializeValues();
  void WhyNotReady();
};
}
