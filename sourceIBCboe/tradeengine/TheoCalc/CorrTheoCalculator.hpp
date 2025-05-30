#pragma once

#include "tradeengine/Executioner/BaseExecutioner.hpp"
#include "tradeengine/TheoCalc/MasterTheoCalculator.hpp"
#include "tradeengine/TheoCalc/SquareOffTheoCalculator.hpp"

class CorrTheoCalculator : public MasterTheoCalculator {
 protected:
  std::vector<std::vector<double> > corr_vec_;
  std::vector<std::vector<double> > return_vec_;
  std::vector<RatioTheoCalculator*> theo_vec_;
  double corr_filter_;
  double num_prod_to_take_;
  void LoadParams();

 public:
  CorrTheoCalculator(std::map<std::string, std::string>* key_val_map, HFSAT::Watch& _watch_,
                       HFSAT::DebugLogger& dbglogger_, int _trading_start_utc_mfm_, int _trading_end_utc_mfm_,
                       int _aggressive_get_flat_mfm);

  virtual ~CorrTheoCalculator() {}

  void ConfigureMidTermDetails(std::map<std::string, BaseTheoCalculator*>& theo_map_);

  double ComputeCorrelation(int index1_, int index2_);
};
