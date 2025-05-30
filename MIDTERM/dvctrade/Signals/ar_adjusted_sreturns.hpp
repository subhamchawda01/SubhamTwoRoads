/**
    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
  Old Madras Road, Near Garden City College,
  KR Puram, Bangalore 560049, India
  +91 80 4190 3551
*/

#pragma once

#include "dvctrade/Signals/base_signal.hpp"
#include <boost/circular_buffer.hpp>

namespace HFSAT {
  // arima adjusted stationary portfolio returns
  
  class ParamSet_ARAdjustedSPortReturns : public ParamSet_BaseSignal {
  public:

    // signal formulation paramaters
    std::vector<int> lags_in_seconds_vec_; // lag to computes returns 120 300 600 900
    std::vector<double> betas_vec_; // ols to make construct stationary series out of instruemnts returns
    std::vector<double> alphas_vec_; // fitting ols residuals using auto-correlation property, AR(1)
    std::vector<double> epsilons_vec_; // expected stdev values of remaining white-noise

    ParamSet_ARAdjustedSPortReturns(const std::string& _filename_, const int _tradingdate_);
    void LoadParamSet(const std::string& _filename_, const int _tradingdate_);
    void ToString();
  };

  class Signal_ARAdjustedSPortReturns : public Signal_BaseSignal {
  public:
    Signal_ARAdjustedSPortReturns(DebugLogger& _dbglogger_, const Watch& _watch_,
				  const ParamSet_ARAdjustedSPortReturns& _paramset_);

    ~Signal_ARAdjustedSPortReturns() {}

    void OnTimePeriodUpdate(const int num_pages_to_add);


    const ParamSet_ARAdjustedSPortReturns& signal_paramset_;
    unsigned int num_updates_;
    unsigned int num_shortcodes_;
    unsigned int num_lags_;
    
    // circular buffur of vec
    boost::circular_buffer<std::vector<double>> streaming_data_;
    std::vector<double> inst_prices_;    
    unsigned int hist_length_;
    std::vector<unsigned int> lag_idxs_;
    boost::circular_buffer<std::vector<double>> last_residuals_;
    std::vector<double> residuals_;
  };
}
