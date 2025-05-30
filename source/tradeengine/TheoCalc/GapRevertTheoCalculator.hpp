#pragma once

#include "tradeengine/Executioner/BaseExecutioner.hpp"
#include "tradeengine/TheoCalc/MidTermTheoCalculator.hpp"
#include "tradeengine/TheoCalc/SquareOffTheoCalculator.hpp"
#include "tradeengine/Utils/BarGenerator.hpp"

/*! \brief Header file for MomentumTheo
*/
class GapRevertTheoCalculator : public MidTermTheoCalculator {
 protected:
  int current_directional_cascade_;
  double open_bar_close_px_;
  int bar_count_;
  double open_obv_max_;
  double open_move_threshold_;
  double current_obv_;
  double last_exec_obv_;
  double min_close_;
  double max_close_;
  int num_bars_;
  int trade_bar_;
  int mid_bar_counter_;
  void LoadParams();
  void onBarUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_);
  void updateTrailingSL(double _price_);


 public:
  GapRevertTheoCalculator(std::map<std::string, std::string>* key_val_map, HFSAT::Watch& _watch_,
                         HFSAT::DebugLogger& dbglogger_, int _trading_start_utc_mfm_, int _trading_end_utc_mfm_,
                         int _aggressive_get_flat_mfm, int eff_squareoff_start_utc_mfm_, double bid_multiplier_,
                         double ask_multiplier_);

  virtual ~GapRevertTheoCalculator() {}

  // void SetupPNLHooks();
  virtual int total_pnl() { return total_pnl_; }
  void ReloadConfig();
  void OnMarketUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo &_market_update_info_);
};
