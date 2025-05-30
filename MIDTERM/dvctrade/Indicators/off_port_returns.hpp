/**
   \file Indicators/off_port_returns.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#pragma once
#include <string>
#include <sys/stat.h>
#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/simple_returns.hpp"

namespace HFSAT {

class OffPortReturns : public CommonIndicator, public IndicatorListener {
 protected:
  // variables
  std::string portfolio_;
  std::string shortcode_;
  std::string sourcealgo_;
  double returns_duration_;
  int lookback_days_;
  PriceType_t price_type_;
  std::vector<std::string> shortcode_vec_;
  std::vector<SimpleReturns*> returns_indicator_vec_;
  std::vector<SecurityMarketView*> indep_market_view_vec_;
  int total_products_;
  int file_extraction_day_;
  std::vector<double> beta_vector_;
  std::vector<double> prev_return_vec_;
  std::vector<bool> is_ready_vec_;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static OffPortReturns* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                           const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static OffPortReturns* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                           const std::string& _portfolio_, const std::string& _shortcode_,
                                           const std::string& _sourcealgo_, double _fractional_seconds_,
                                           int _lookback_days_, PriceType_t _price_type_);

 protected:
  OffPortReturns(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
                 const std::string& _portfolio_, const std::string& _shortcode_, const std::string& _sourcealgo_,
                 double _fractional_seconds_, int _lookback_days_, PriceType_t _price_type_);
  std::string GetLatestFile(const std::string& path_);
  inline bool DoesFileExist(const std::string& path_) {
    struct stat buffer;
    return (stat(path_.c_str(), &buffer) == 0);
  }
  void CheckForReady();

 public:
  ~OffPortReturns() {}

  // listener interface
  inline void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {}
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }

  void OnPortfolioPriceChange(double _new_price_){};
  void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

  inline void SubscribeDataInterrupts(MarketUpdateManager& market_update_manager_) {}

  static std::string VarName() { return "OffPortReturns"; }

  void WhyNotReady();

  inline void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  inline void OnMarketDataResumed(const unsigned int _security_id_);

  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_);
  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }

 protected:
  void Initialize();
  void AddIndicatorListener();
};
}
