#pragma once

#include "tradeengine/Executioner/BaseExecutioner.hpp"
#include "tradeengine/TheoCalc/MidTermTheoCalculator.hpp"
#include "tradeengine/TheoCalc/SquareOffTheoCalculator.hpp"
#include "tradeengine/Utils/BarGenerator.hpp"
/*! \brief Header file for MACDTheo
*/
class MACDTheoCalculator : public MidTermTheoCalculator {
 protected:
  int sema_param_;
  int lema_param_;
  int signal_param_;
  int bollinger_period_;
  int num_bars_;
  double day_bollinger_;
  double sema_;
  double lema_;
  double macd_;
  double signal_;
  double prev_sema_;
  double prev_lema_;
  double prev_macd_;
  double prev_signal_;
  int last_crossover_;
  int macd_crossover_lkbk_;
  int current_directional_cascade_;
  void LoadParams();
  void onBarUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_);
  void updateTrailingSL(double _price_);

 public:
  MACDTheoCalculator(std::map<std::string, std::string>* key_val_map, HFSAT::Watch& _watch_,
                     HFSAT::DebugLogger& dbglogger_, int _trading_start_utc_mfm_, int _trading_end_utc_mfm_,
                     int _aggressive_get_flat_mfm, int eff_squareoff_start_utc_mfm_, double bid_multiplier_,
                     double ask_multiplier_);

  virtual ~MACDTheoCalculator() {}

  // void SetupPNLHooks();
  virtual int total_pnl() { return total_pnl_; }
  void ReloadConfig();
};