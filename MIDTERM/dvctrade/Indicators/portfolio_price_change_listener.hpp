/**
    \file Indicators/portfolio_price_change_listener.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_PORTFOLIO_PRICE_CHANGE_LISTENER_H
#define BASE_INDICATORS_PORTFOLIO_PRICE_CHANGE_LISTENER_H

namespace HFSAT {

/// Listener interface for SimpleTrendPort sort of variables, which listen to an aggregated price,
class PortfolioPriceChangeListener {
 public:
  virtual ~PortfolioPriceChangeListener(){};
  virtual void OnPortfolioPriceChange(double _new_price_) = 0;
  virtual void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) = 0;
  // is_data_interrupted_ 0 = do what we were doing earlier; 1 = data interrupt has come; 2 = data interrupt has been
  // resolved
};
}

#endif  // BASE_INDICATORS_PORTFOLIO_PRICE_CHANGE_LISTENER_H
