/**
    \file testbest/cpptest52.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/

/*
  The optimization marked under TRYNEW here reduces cycles from 46.58 to 14.26 for the three assignments.
 */

#include <stdio.h>

#include "dvccode/Profiler/cpucycle_profiler.hpp"

typedef enum {
  kPriceTypeMidprice,       // 0
  kPriceTypeMktSizeWPrice,  // 1
  kPriceTypeMktSinusoidal,  // 2
  kPriceTypeOfflineMixMMS,  // 3
  kPriceTypeMax             // 4
} PriceType_t;

struct MarketUpdateInfo {
#ifdef TRYNEW
  union {
    double prices_[kPriceTypeMax];
    struct {
#endif
      double mid_price_;
      double mkt_size_weighted_price_;
      double mkt_sinusoidal_price_;
      double offline_mix_mms_price_;
#ifdef TRYNEW
    };
  };
#endif
};

static inline double& GetPriceFromType(const PriceType_t t_price_type_, MarketUpdateInfo& this_market_update_info_) {
  switch (t_price_type_) {
    case kPriceTypeMidprice:
      return this_market_update_info_.mid_price_;
      break;
    case kPriceTypeMktSizeWPrice:
      return this_market_update_info_.mkt_size_weighted_price_;
      break;
    case kPriceTypeMktSinusoidal:
      return this_market_update_info_.mkt_sinusoidal_price_;
      break;
    default:
      return this_market_update_info_.mid_price_;
  }
  return this_market_update_info_.mid_price_;
}

int main() {
  MarketUpdateInfo abc_;
  HFSAT::CpucycleProfiler& cpucycle_profiler_ = HFSAT::CpucycleProfiler::SetUniqueInstance(1);

  cpucycle_profiler_.Start(0);

  for (auto i = 0u; i < 1000000; i++) {
#ifdef TRYNEW
    abc_.prices_[kPriceTypeMidprice] = i;
    abc_.prices_[kPriceTypeMktSinusoidal] = i;
    abc_.prices_[kPriceTypeMktSizeWPrice] = i;
#else
        GetPriceFromType(kPriceTypeMidprice, abc_) == i;
        GetPriceFromType(kPriceTypeMktSinusoidal, abc_) == i;
        GetPriceFromType(kPriceTypeMktSizeWPrice, abc_) == i;
#endif
  }

  cpucycle_profiler_.End(0);

  std::vector<HFSAT::CpucycleProfilerSummaryStruct> cpucycle_summary_vec_ = cpucycle_profiler_.GetCpucycleSummary();
  for (auto i = 0u; i < cpucycle_summary_vec_.size(); i++) {
    fprintf(stderr, "%d:\t Min:%u\t Max:%u\t Mean:%u Median:%u\n", i, cpucycle_summary_vec_[i].min_,
            cpucycle_summary_vec_[i].max_, cpucycle_summary_vec_[i].mean_, cpucycle_summary_vec_[i].fifty_percentile_);
  }

  return 0;
}
