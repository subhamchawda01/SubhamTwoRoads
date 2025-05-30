#pragma once

#include "tradeengine/Executioner/BaseExecutioner.hpp"
#include "tradeengine/TheoCalc/MidTermTheoCalculator.hpp"
#include "tradeengine/TheoCalc/SquareOffTheoCalculator.hpp"
#include "tradeengine/Utils/BarGenerator.hpp"

/*! \brief Header file for MomentumTheo
*/
class MATheoCalculator : public MidTermTheoCalculator {
 protected:
  std::vector<double> day_moving_avg_{0,0,0,0};
  std::vector<int>  current_directional_cascade_{0,0,0,0};
  int crossover_threshold_;
  double ma_entry_thesh_;
  std::vector<unsigned int> day_ma_lkbk_{30,50,100,200};
  std::vector<double> crossover_lkbk_{1000,1000,1000,1000};
  std::vector<double> support_vec_{0,0,0,0};
  std::vector<double> resist_vec_{10000000,10000000,10000000,10000000};
  std::vector<double> cascade_px_vec_{0,0,0,0};
  std::vector<double> midterm_pnl_vec_{0,0,0,0};
  std::vector<double> prev_pnl_vec_{0,0,0,0};
  std::vector<int> ma_pos_offset_{0,0,0,0};
  std::vector<bool> cascade_condn_{false,false,false,false};
  std::vector<bool> sqoff_flag_{false,false,false,false};
  // int day_ma_lkbk_[];
  void LoadParams();
  void onBarUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_);
  void updateTrailingSL(double _price_);

 public:
  MATheoCalculator(std::map<std::string, std::string>* key_val_map, HFSAT::Watch& _watch_,
                         HFSAT::DebugLogger& dbglogger_, int _trading_start_utc_mfm_, int _trading_end_utc_mfm_,
                         int _aggressive_get_flat_mfm, int eff_squareoff_start_utc_mfm_, double bid_multiplier_,
                         double ask_multiplier_);

  virtual ~MATheoCalculator() {}

  // void SetupPNLHooks();
  virtual int total_pnl() { return total_pnl_; }
  void ReloadConfig();
};
